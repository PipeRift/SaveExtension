// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "SlotDataTask_Saver.h"

#include <Kismet/GameplayStatics.h>
#include <Engine/LocalPlayer.h>
#include <GameFramework/GameModeBase.h>
#include <GameFramework/GameStateBase.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>
#include <GameFramework/HUD.h>
#include <Serialization/MemoryWriter.h>
#include <Components/CapsuleComponent.h>

#include "SaveManager.h"
#include "SlotInfo.h"
#include "SlotData.h"
#include "SavePreset.h"
#include "FileAdapter.h"


/////////////////////////////////////////////////////
// FSerializeActorsTask
void FSerializeActorsTask::DoWork()
{
	bool bIsAIController;
	bool bIsLevelScript;

	if (bIsSync)
	{
		if (bStoreGameInstance)
			SerializeGameInstance();

		if (bStoreGameMode)
		{
			SerializeGameMode();
			SerializeGameState();

			if (World->GetGameInstance()->GetLocalPlayers().Num() > 0)
			{
				const int32 PlayerId = 0;

				SerializePlayerState(PlayerId);
				SerializePlayerController(PlayerId);
				//SerializePlayerPawn(PlayerId);
				SerializePlayerHUD(PlayerId);
			}
		}
	}

	for (int32 I = 0; I < Num; ++I)
	{
		const AActor* const Actor = (*LevelActors)[StartIndex + I];
		if (ShouldSave(Actor))
		{
			bIsAIController = false;
			bIsLevelScript = false;

			if (ShouldSaveAsWorld(Actor, bIsAIController, bIsLevelScript))
			{
				if (bIsAIController)
				{
					if (const AAIController* const AI = Cast<AAIController>(Actor))
					{
						FControllerRecord Record;
						SerializeController(AI, Record);
						AIControllerRecords.Add(MoveTemp(Record));
					}
				}
				else if (bIsLevelScript)
				{
					if (const ALevelScriptActor* const LevelScript = Cast<ALevelScriptActor>(Actor))
					{
						SerializeActor(LevelScript, LevelScriptRecord);
					}
				}
				else
				{
					FActorRecord Record;
					SerializeActor(Actor, Record);
					ActorRecords.Add(MoveTemp(Record));
				}
			}
		}
	}
}

bool FSerializeActorsTask::SerializeController(const AController* Actor, FControllerRecord& Record) const
{
	const bool bResult = SerializeActor(Actor, Record);
	if (bResult && bStoreControlRotation)
	{
		Record.ControlRotation = Actor->GetControlRotation();
	}
	return bResult;
}

void FSerializeActorsTask::SerializeGameMode()
{
	if (ShouldSave(World->GetAuthGameMode()))
	{
		SerializeActor(World->GetAuthGameMode(), SlotData->GameMode);
	}
}

void FSerializeActorsTask::SerializeGameState()
{
	const auto* GameState = World->GetGameState();
	if (ShouldSave(GameState))
	{
		SerializeActor(GameState, SlotData->GameState);
	}
}

void FSerializeActorsTask::SerializePlayerState(int32 PlayerId)
{
	const auto* Controller = UGameplayStatics::GetPlayerController(World, PlayerId);
	if (Controller && ShouldSave(Controller->PlayerState))
	{
		SerializeActor(Controller->PlayerState, SlotData->PlayerState);
	}
}

void FSerializeActorsTask::SerializePlayerController(int32 PlayerId)
{
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, PlayerId);
	if (ShouldSave(PlayerController))
	{
		SerializeController(PlayerController, SlotData->PlayerController);
	}
}

void FSerializeActorsTask::SerializePlayerHUD(int32 PlayerId)
{
	const auto* Controller = UGameplayStatics::GetPlayerController(World, PlayerId);
	if (Controller && ShouldSave(Controller->MyHUD))
	{
		SerializeActor(Controller->MyHUD, SlotData->PlayerHUD);
	}
}

void FSerializeActorsTask::SerializeGameInstance()
{
	UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance)
	{
		FObjectRecord Record{ GameInstance };

		//Serialize into Record Data
		FMemoryWriter MemoryWriter(Record.Data, true);
		FSaveExtensionArchive Archive(MemoryWriter, false);
		GameInstance->Serialize(Archive);

		SlotData->GameInstance = MoveTemp(Record);
	}
}

bool FSerializeActorsTask::SerializeActor(const AActor* Actor, FActorRecord& Record) const
{

	//Clean the record
	Record = { Actor };

	Record.bHiddenInGame = Actor->bHidden;
	Record.bIsProcedural = IsProcedural(Actor);

	if (SavesTags(Actor))
	{
		Record.Tags = Actor->Tags;
	}
	else
	{
		// Only save save-tags
		for (const auto& Tag : Actor->Tags)
		{
			if (USlotDataTask::IsSaveTag(Tag))
			{
				Record.Tags.Add(Tag);
			}
		}
	}

	if (SavesTransform(Actor))
	{
		Record.Transform = Actor->GetTransform();

		if (SavesPhysics(Actor))
		{
			USceneComponent* const Root = Actor->GetRootComponent();
			if (Root && Root->Mobility == EComponentMobility::Movable)
			{
				if (auto* const Primitive = Cast<UPrimitiveComponent>(Root))
				{
					Record.LinearVelocity = Primitive->GetPhysicsLinearVelocity();
					Record.AngularVelocity = Primitive->GetPhysicsAngularVelocityInRadians();
				}
				else
				{
					Record.LinearVelocity = Root->GetComponentVelocity();
				}
			}
		}
	}

	if (SavesComponents(Actor))
	{
		SerializeActorComponents(Actor, Record, 1);
	}

	FMemoryWriter MemoryWriter(Record.Data, true);
	FSaveExtensionArchive Archive(MemoryWriter, false);
	const_cast<AActor*>(Actor)->Serialize(Archive);

	return true;
}

void FSerializeActorsTask::SerializeActorComponents(const AActor* Actor, FActorRecord& ActorRecord, int8 indent /*= 0*/) const
{
	const TSet<UActorComponent*>& Components = Actor->GetComponents();
	for (auto* Component : Components)
	{
		if (ShouldSave(Component))
		{
			FComponentRecord ComponentRecord;
			ComponentRecord.Name = Component->GetFName();
			ComponentRecord.Class = Component->GetClass();

			if (SavesTransform(Component))
			{
				const USceneComponent* Scene = CastChecked<USceneComponent>(Component);
				if (Scene->Mobility == EComponentMobility::Movable)
				{
					ComponentRecord.Transform = Scene->GetRelativeTransform();
				}
			}

			if (SavesTags(Component))
			{
				ComponentRecord.Tags = Component->ComponentTags;
			}

			if (!Component->GetClass()->IsChildOf<UPrimitiveComponent>())
			{
				FMemoryWriter MemoryWriter(ComponentRecord.Data, true);
				FSaveExtensionArchive Archive(MemoryWriter, false);
				Component->Serialize(Archive);
			}
			ActorRecord.ComponentRecords.Add(ComponentRecord);
		}
	}
}


/////////////////////////////////////////////////////
// USaveDataTask_Saver

void USlotDataTask_Saver::OnStart()
{
	USaveManager* Manager = GetManager();
	Manager->TryInstantiateInfo();

	bool bSave = true;
	const FString InfoCard = Manager->GenerateSlotInfoName(Slot);
	const FString DataCard = Manager->GenerateSlotDataName(Slot);

	//Overriding
	{
		const bool bInfoExists = FFileAdapter::DoesFileExist(InfoCard);
		const bool bDataExists = FFileAdapter::DoesFileExist(DataCard);

		if (bOverride)
		{
			// Delete previous save
			if (bInfoExists)
			{
				FFileAdapter::DeleteFile(InfoCard);
			}
			if (bDataExists)
			{
				FFileAdapter::DeleteFile(DataCard);
			}
		}
		else
		{
			//Only save if previous files don't exist
			//We don't want to serialize since it won't be saved anyway
			bSave = !bInfoExists && !bDataExists;
		}
	}

	if (bSave)
	{
		const UWorld* World = GetWorld();

		GetManager()->OnSaveBegan();

		USlotInfo* CurrentInfo = Manager->GetCurrentInfo();
		SlotData = Manager->GetCurrentData();
		SlotData->Clean(true);


		check(CurrentInfo && SlotData);

		const bool bSlotWasDifferent = CurrentInfo->Id != Slot;
		CurrentInfo->Id = Slot;

		if (bSaveThumbnail)
		{
			CurrentInfo->CaptureThumbnail(Width, Height);
		}

		// Time stats
		{
			CurrentInfo->SaveDate = FDateTime::Now();

			// If this info has been loaded ever
			const bool bWasLoaded = CurrentInfo->LoadDate.GetTicks() > 0;
			if (bWasLoaded)
			{
				// Now - Loaded
				const FTimespan SessionTime = CurrentInfo->SaveDate - CurrentInfo->LoadDate;

				CurrentInfo->PlayedTime += SessionTime;

				if (!bSlotWasDifferent)
					CurrentInfo->SlotPlayedTime += SessionTime;
				else
					CurrentInfo->SlotPlayedTime = SessionTime;
			}
			else
			{
				// Slot is new, played time is world seconds
				CurrentInfo->PlayedTime = FTimespan::FromSeconds(World->TimeSeconds);
				CurrentInfo->SlotPlayedTime = CurrentInfo->PlayedTime;
			}

			// Save current game seconds
			SlotData->TimeSeconds = World->TimeSeconds;
		}

		//Save Level info in both files
		CurrentInfo->Map = World->GetFName();
		SlotData->Map = World->GetFName().ToString();

		SerializeSync();
		SaveFile(InfoCard, DataCard);
		return;
	}
	Finish(false);
}

void USlotDataTask_Saver::OnFinish(bool bSuccess)
{
	if (bSuccess)
	{
		// Clean serialization data
		SlotData->Clean(true);

		SELog(Preset, "Finished Saving", FColor::Green);
	}

	// Execute delegates
	USaveManager* Manager = GetManager();
	check(Manager);
	Delegate.ExecuteIfBound((Manager && bSuccess)? Manager->GetCurrentInfo() : nullptr);
	Manager->OnSaveFinished(!bSuccess);
}

void USlotDataTask_Saver::BeginDestroy()
{
	if (SaveInfoTask) {
		SaveInfoTask->EnsureCompletion(false);
		delete SaveInfoTask;
	}

	if (SaveDataTask) {
		SaveDataTask->EnsureCompletion(false);
		delete SaveDataTask;
	}

	Super::BeginDestroy();
}

void USlotDataTask_Saver::SerializeSync()
{
	// Has Authority
	if (GetWorld()->GetAuthGameMode())
	{
		// Save World
		SerializeWorld();
	}
}

void USlotDataTask_Saver::SerializeWorld()
{
	const UWorld* World = GetWorld();

	SELog(Preset, "World '" + World->GetName() + "'", FColor::Green, false, 1);

	const TArray<ULevelStreaming*>& Levels = World->GetStreamingLevels();

	// Threads available + 1 (Synchronous Thread)
	const int32 NumberOfThreads = FMath::Max(1, FPlatformMisc::NumberOfWorkerThreadsToSpawn() + 1);
	const int32 TasksPerLevel = FMath::Max(1, FMath::RoundToInt(float(NumberOfThreads) / (Levels.Num() + 1)));

	Tasks.Reserve(NumberOfThreads);

	SerializeLevelSync(World->GetCurrentLevel(), TasksPerLevel);
	for (const ULevelStreaming* Level : Levels)
	{
		if (Level->IsLevelLoaded())
		{
			SerializeLevelSync(Level->GetLoadedLevel(), TasksPerLevel, Level);
		}
	}

	RunScheduledTasks();
}

void USlotDataTask_Saver::SerializeLevelSync(const ULevel* Level, int32 AssignedTasks, const ULevelStreaming* StreamingLevel)
{
	check(IsValid(Level));

	if (!Preset->IsMTSerializationSave())
		AssignedTasks = 1;

	const FName LevelName = StreamingLevel ? StreamingLevel->GetWorldAssetPackageFName() : FPersistentLevelRecord::PersistentName;
	SELog(Preset, "Level '" + LevelName.ToString() + "'", FColor::Green, false, 1);


	// Find level record. By default, main level
	FLevelRecord* LevelRecord = &SlotData->MainLevel;
	if (StreamingLevel)
	{
		// Find or create the sub-level
		const int32 Index = SlotData->SubLevels.AddUnique({ StreamingLevel });
		LevelRecord = &SlotData->SubLevels[Index];
	}
	check(LevelRecord);

	// Empty level record before serializing it
	LevelRecord->Clean();

	const int32 ActorCount = Level->Actors.Num();
	const int32 ObjectsPerTask = FMath::CeilToInt((float)ActorCount / AssignedTasks);

	// Split all actors between multi-threaded tasks
	int32 Index{ 0 };
	while (Index < ActorCount)
	{
		// Add new Task
		const bool IsFirstTask = Index <= 0;
		Tasks.Emplace(IsFirstTask, GetWorld(), SlotData, &Level->Actors, Index, ObjectsPerTask, LevelRecord, Preset);
		Index += ObjectsPerTask;
	}
}

void USlotDataTask_Saver::RunScheduledTasks()
{
	// Start all serialization tasks
	if (Tasks.Num() > 0)
	{
		Tasks[0].StartSynchronousTask();
		for (int32 I = 1; I < Tasks.Num(); ++I)
		{
			if (Preset->IsMTSerializationSave())
				Tasks[I].StartBackgroundTask();
			else
				Tasks[I].StartSynchronousTask();
		}
	}
	// Wait until all tasks have finished
	for (auto& AsyncTask : Tasks)
	{
		AsyncTask.EnsureCompletion();
	}
	// All tasks finished, sync data
	for (auto& AsyncTask : Tasks)
	{
		AsyncTask.GetTask().DumpData();
	}
	Tasks.Empty();
}

void USlotDataTask_Saver::SaveFile(const FString& InfoName, const FString& DataName)
{
	USaveManager* Manager = GetManager();

	USlotInfo* CurrentInfo = Manager->GetCurrentInfo();
	USlotData* CurrentData = Manager->GetCurrentData();

	SaveInfoTask = new FAsyncTask<FSaveFileTask>(CurrentInfo, InfoName, false /* Infos don't use compression to be loaded faster */);
	SaveDataTask = new FAsyncTask<FSaveFileTask>(CurrentData, DataName, Preset->bUseCompression);

	if (Preset->IsMTFilesSave())
	{
		SaveInfoTask->StartBackgroundTask();
		SaveDataTask->StartBackgroundTask();
	}
	else
	{
		SaveInfoTask->StartSynchronousTask();
		SaveDataTask->StartSynchronousTask();
		Finish(true);
	}
}
