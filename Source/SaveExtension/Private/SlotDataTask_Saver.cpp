// Copyright 2015-2018 Piperift. All Rights Reserved.

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
	for (int32 I = 0; I < Num; ++I)
	{
		const AActor* const Actor = (*LevelActors)[StartIndex + I];
		if (ShouldSave(Actor))
		{
			if (const AAIController* const AI = Cast<AAIController>(Actor))
			{
				if (bStoreAIControllers)
				{
					FControllerRecord Record;
					SerializeController(AI, Record);
					AIControllerRecords.Add(MoveTemp(Record));
				}
			}
			else if (const ALevelScriptActor* const LevelScript = Cast<ALevelScriptActor>(Actor))
			{
				if (bStoreLevelBlueprints) {
					SerializeActor(LevelScript, LevelScriptRecord);
				}
			}
			else if (ShouldSaveAsWorld(Actor))
			{
				FActorRecord Record;
				SerializeActor(Actor, Record);
				ActorRecords.Add(MoveTemp(Record));
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
			if (IsSaveTag(Tag))
			{
				Record.Tags.Add(Tag);
			}
		}
	}

	const bool bSavesPhysics = SavesPhysics(Actor);
	if (SavesTransform(Actor) || bSavesPhysics)
	{
		Record.Transform = Actor->GetTransform();
	}

	if (bSavesPhysics)
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
	const FString InfoCard = Manager->GenerateSaveSlotName(Slot);
	const FString DataCard = Manager->GenerateSaveDataSlotName(Slot);

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
		GetManager()->OnSaveBegan();

		USlotInfo* CurrentInfo = Manager->GetCurrentInfo();
		SlotData = Manager->GetCurrentData();
		SlotData->Clean(false);


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

		// Clean serialization data
		SlotData->Clean(true);

		SELog(Preset, "Finished Saving", FColor::Green);
	}
	GetManager()->OnSaveFinished(false);
	Finish(bSave);
}

void USlotDataTask_Saver::SerializeSync()
{
	// Save World
	SerializeWorld();

	if (Preset->bStoreGameInstance)
		SerializeGameInstance();

	if (Preset->bStoreGameMode)
	{
		SerializeGameMode();
		SerializeGameState();

		const TArray<ULocalPlayer*>& LocalPlayers = World->GetGameInstance()->GetLocalPlayers();
		for (const auto* LocalPlayer : LocalPlayers)
		{
			int32 PlayerId = LocalPlayer->GetControllerId();

			SerializePlayerState(PlayerId);
			SerializePlayerController(PlayerId);
			//SerializePlayerPawn(PlayerId);
			SerializePlayerHUD(PlayerId);
		}
	}
}

void USlotDataTask_Saver::SerializeWorld()
{
	check(World);
	SELog(Preset, "World '" + World->GetName() + "'", FColor::Green, false, 1);

	const TArray<ULevelStreaming*>& Levels = World->GetStreamingLevels();

	const int32 NumberOfThreads = FMath::Max(1, FPlatformMisc::NumberOfWorkerThreadsToSpawn());
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

	// Start all serialization tasks
	if (Tasks.Num() > 0)
	{
		Tasks[0].StartSynchronousTask();
		for (int32 I = 1; I < Tasks.Num(); ++I)
		{
			Tasks[I].StartBackgroundTask();
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

void USlotDataTask_Saver::SerializeLevelSync(const ULevel* Level, const int32 AssignedTasks, const ULevelStreaming* StreamingLevel)
{
	check(IsValid(Level));

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

	// Split all actors between async tasks
	int32 StartIndex{ 0 };
	while (StartIndex < ActorCount)
	{
		Tasks.Emplace(&Level->Actors, StartIndex, ObjectsPerTask, LevelRecord, Preset);
		StartIndex += ObjectsPerTask;
	}
}

void USlotDataTask_Saver::SerializeLevelScript(const ALevelScriptActor* Level, FLevelRecord& LevelRecord) const
{
	if (Preset->bStoreLevelBlueprints)
	{
		check(Level);

		SerializeActor(Level, LevelRecord.LevelScript);

		SELog(Preset, "Level Blueprint '" + Level->GetName() + "'", FColor::White, false, 2);
	}
}

void USlotDataTask_Saver::SerializeAI(const AAIController* AIController, FLevelRecord& LevelRecord) const
{
	if (Preset->bStoreAIControllers)
	{
		check(AIController);

		FControllerRecord Record;
		SerializeController(AIController, Record);
		LevelRecord.AIControllers.Add(Record);

		SELog(Preset, "AI Controller '" + AIController->GetName() + "'", FColor::White, 1);
	}
}

void USlotDataTask_Saver::SerializeGameMode()
{
	if (ShouldSave(World->GetAuthGameMode()))
	{
		FActorRecord& Record = SlotData->GameMode;
		const bool bSuccess = SerializeActor(World->GetAuthGameMode(), Record);

		SELog(Preset, "Game Mode '" + Record.Name.ToString() + "'", FColor::Green, !bSuccess, 1);
	}
}

void USlotDataTask_Saver::SerializeGameState()
{
	const auto* GameState = World->GetGameState();
	if (ShouldSave(GameState))
	{
		FActorRecord& Record = SlotData->GameState;
		SerializeActor(GameState, Record);

		SELog(Preset, "Game State '" + Record.Name.ToString() + "'", 1);
	}
}

void USlotDataTask_Saver::SerializePlayerState(int32 PlayerId)
{
	const auto* Controller = UGameplayStatics::GetPlayerController(World, PlayerId);
	if (Controller && ShouldSave(Controller->PlayerState))
	{
		FActorRecord& Record = SlotData->PlayerState;
		const bool bSuccess = SerializeActor(Controller->PlayerState, Record);

		SELog(Preset, "Player State '" + Record.Name.ToString() + "'", FColor::Green, !bSuccess, 1);
	}
}

void USlotDataTask_Saver::SerializePlayerController(int32 PlayerId)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, PlayerId);
	if (ShouldSave(PlayerController))
	{
		FControllerRecord& Record = SlotData->PlayerController;
		const bool bSuccess = SerializeController(PlayerController, Record);

		SELog(Preset, "Player Controller '" + Record.Name.ToString() + "'", FColor::Green, !bSuccess, 1);
	}
}

void USlotDataTask_Saver::SerializePlayerHUD(int32 PlayerId)
{
	const auto* Controller = UGameplayStatics::GetPlayerController(World, PlayerId);
	if (Controller && ShouldSave(Controller->MyHUD))
	{
		FActorRecord& Record = SlotData->PlayerHUD;
		const bool bSuccess = SerializeActor(Controller->MyHUD, Record);

		SELog(Preset, "Player HUD '" + Record.Name.ToString() + "'", FColor::Green, !bSuccess, 1);
	}
}

void USlotDataTask_Saver::SerializeGameInstance()
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(World);
	if (!GameInstance)
	{
		SELog(Preset, "Game Instance - No Game Instance Found", FColor::White, true, 1);
		return;
	}

	FObjectRecord Record { GameInstance };

	//Serialize into Record Data
	FMemoryWriter MemoryWriter(Record.Data, true);
	FSaveExtensionArchive Archive(MemoryWriter, false);
	GameInstance->Serialize(Archive);

	SlotData->GameInstance = MoveTemp(Record);

	SELog(Preset, "Game Instance '" + Record.Name.ToString() + "'", FColor::White, 1);
}

bool USlotDataTask_Saver::SerializeActor(const AActor* Actor, FActorRecord& Record) const
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
			if (IsSaveTag(Tag))
			{
				Record.Tags.Add(Tag);
			}
		}
	}

	const bool bSavesPhysics = SavesPhysics(Actor);
	if (SavesTransform(Actor) || bSavesPhysics)
	{
		Record.Transform = Actor->GetTransform();
	}

	if (bSavesPhysics)
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

	if (SavesComponents(Actor))
	{
		SerializeActorComponents(Actor, Record, 1);
	}

	FMemoryWriter MemoryWriter(Record.Data, true);
	FSaveExtensionArchive Archive(MemoryWriter, false);
	const_cast<AActor*>(Actor)->Serialize(Archive);

	return true;
}

bool USlotDataTask_Saver::SerializeController(const AController* Actor, FControllerRecord& Record) const
{
	const bool bResult = SerializeActor(Actor, Record);
	if (bResult && Preset->bStoreControlRotation)
	{
		Record.ControlRotation = Actor->GetControlRotation();
	}
	return bResult;
}

void USlotDataTask_Saver::SerializeActorComponents(const AActor* Actor, FActorRecord& ActorRecord, int8 Indent) const
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

bool USlotDataTask_Saver::SaveFile(const FString& InfoName, const FString& DataName) const
{
	USaveManager* Manager = GetManager();

	USlotInfo* CurrentInfo = Manager->GetCurrentInfo();
	USlotData* CurrentData = Manager->GetCurrentData();

	if (FFileAdapter::SaveFile(CurrentInfo, InfoName, Preset) &&
		FFileAdapter::SaveFile(CurrentData, DataName, Preset))
	{
		return true;
	}

	return false;
}
