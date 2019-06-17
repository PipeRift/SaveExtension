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
					if (const auto* const AI = Cast<AAIController>(Actor))
					{
						FControllerRecord Record;
						SerializeController(AI, Record);
						AIControllerRecords.Add(MoveTemp(Record));
					}
				}
				else if (bIsLevelScript)
				{
					if (const auto* const LevelScript = Cast<ALevelScriptActor>(Actor))
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
		FSEArchive Archive(MemoryWriter, false);
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
	FSEArchive Archive(MemoryWriter, false);
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
				FSEArchive Archive(MemoryWriter, false);
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

	const FString InfoCard = Manager->GenerateSlotInfoName(Slot);
	const FString DataCard = Manager->GenerateSlotDataName(Slot);

	if (!TryOverridePreviousSlot(InfoCard, DataCard))
	{
		Finish(false);
		return;
	}

	SlotInfo = Manager->GetCurrentInfo();
	SlotData = Manager->GetCurrentData();
	SlotData->Clean(true); // Reset previously stored data but keep level data

	GetManager()->OnSaveBegan();

	Prepare();
	UpdateInfoStats();

	//Save Level info
	SlotData->Map = GetWorld()->GetFName().ToString();

	SerializeSync();
	SaveFile(InfoCard, DataCard);
}

void USlotDataTask_Saver::Tick(float DeltaTime)
{
	// If save file tasks exist and are both done
	if (SaveInfoTask && SaveDataTask && SaveInfoTask->IsDone() && SaveDataTask->IsDone())
	{
		Finish(true);
	}
}

void USlotDataTask_Saver::OnFinish(bool bSuccess)
{
	if (bSuccess)
	{
		// Clean serialization data
		SlotData->Clean(true);

		SELog(*Settings, "Finished Saving", FColor::Green);
	}

	// Execute delegates
	USaveManager* Manager = GetManager();
	Delegate.ExecuteIfBound(bSuccess? Manager->GetCurrentInfo() : nullptr);
	Manager->OnSaveFinished(!bSuccess);

	Graph->MarkPendingKill();
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

bool USlotDataTask_Saver::TryOverridePreviousSlot(const FString& InfoCard, const FString& DataCard)
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
		return true;
	}

	//Only save if previous files don't exist
	return !bInfoExists && !bDataExists;
}

bool USlotDataTask_Saver::Prepare()
{
	Graph = NewObject<USaveGraph>(this, GraphClass);

	// Prepare level records before Graph is executed
	const TArray<ULevelStreaming*>& Levels = GetWorld()->GetStreamingLevels();
	TArray<FLevelRecord*> SavedSubLevels;
	for (auto* Level : Levels)
	{
		const int32 Index = SlotData->SubLevels.AddUnique({ Level });
		SavedSubLevels.Add(&SlotData->SubLevels[Index]);
	}

	return Graph->DoPrepare(SlotInfo, SlotData, MoveTemp(SavedSubLevels));
}

void USlotDataTask_Saver::UpdateInfoStats()
{
	const UWorld* World = GetWorld();

	const bool bSlotWasDifferent = SlotInfo->Id != Slot;
	SlotInfo->Id = Slot;
	SlotInfo->Map = World->GetFName();

	if (bSaveThumbnail)
	{
		SlotInfo->CaptureThumbnail(Width, Height);
	}

	// Time Stats
	SlotInfo->SaveDate = FDateTime::Now();

	// If this info has been loaded ever
	const bool bWasLoaded = SlotInfo->LoadDate.GetTicks() > 0;
	if (bWasLoaded)
	{
		// Now - Loaded
		const FTimespan SessionTime = SlotInfo->SaveDate - SlotInfo->LoadDate;

		SlotInfo->PlayedTime += SessionTime;

		if (!bSlotWasDifferent)
			SlotInfo->SlotPlayedTime += SessionTime;
		else
			SlotInfo->SlotPlayedTime = SessionTime;
	}
	else
	{
		// Slot is new, played time is world seconds
		SlotInfo->PlayedTime = FTimespan::FromSeconds(World->TimeSeconds);
		SlotInfo->SlotPlayedTime = SlotInfo->PlayedTime;
	}

	// Save current game seconds
	SlotData->TimeSeconds = World->TimeSeconds;
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

	SELog(*Settings, "World '" + World->GetName() + "'", FColor::Green, false, 1);

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

	if (!Settings->IsMTSerializationSave())
		AssignedTasks = 1;

	const FName LevelName = StreamingLevel ? StreamingLevel->GetWorldAssetPackageFName() : FPersistentLevelRecord::PersistentName;
	SELog(*Settings, "Level '" + LevelName.ToString() + "'", FColor::Green, false, 1);


	// Find level record. By default, main level
	FLevelRecord* LevelRecord = SlotData->FindLevelRecord(StreamingLevel);
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
		Tasks.Emplace(IsFirstTask, GetWorld(), SlotData, &Level->Actors, Index, ObjectsPerTask, LevelRecord, *Settings);
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
			if (Settings->IsMTSerializationSave())
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
	Tasks.Reset();
}

void USlotDataTask_Saver::SaveFile(const FString& InfoName, const FString& DataName)
{
	USaveManager* Manager = GetManager();

	USlotInfo* CurrentInfo = Manager->GetCurrentInfo();
	USlotData* CurrentData = Manager->GetCurrentData();

	SaveInfoTask = new FAsyncTask<FSaveFileTask>(CurrentInfo, InfoName, false /* Infos don't use compression to be loaded faster */);
	SaveDataTask = new FAsyncTask<FSaveFileTask>(CurrentData, DataName, Settings->bUseCompression);

	if (Settings->IsMTFilesSave())
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
