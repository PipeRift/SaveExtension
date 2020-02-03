// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "Serialization/SlotDataTask_Loader.h"

#include <GameFramework/Character.h>
#include <Serialization/MemoryReader.h>
#include <Kismet/GameplayStatics.h>
#include <Components/PrimitiveComponent.h>

#include "SavePreset.h"
#include "SaveManager.h"
#include "Serialization/SEArchive.h"


/////////////////////////////////////////////////////
// USaveDataTask_Loader

void USlotDataTask_Loader::OnStart()
{
	USaveManager* Manager = GetManager();

	SELog(Preset, "Loading from Slot " + FString::FromInt(Slot));

	NewSlotInfo = Manager->LoadInfo(Slot);
	if (!NewSlotInfo)
	{
		SELog(Preset, "Slot Info not found! Can't load.", FColor::White, true, 1);
		Finish(false);
		return;
	}

	// Load data while level loads
	StartLoadingData();

	// Cross-Level loading
	const UWorld* World = GetWorld();
	if (World->GetFName() != NewSlotInfo->Map)
	{
		bLoadingMap = true;

		//Keep loaded Info for loading
		UGameplayStatics::OpenLevel(this, NewSlotInfo->Map);

		SELog(Preset, "Slot '" + FString::FromInt(Slot) + "' is recorded on another Map. Loading before charging slot.", FColor::White, false, 1);
	}
	else if(IsDataLoaded())
	{
		// Will only continue if data has been loaded. Otherwise, it will check on tick
		StartDeserialization();
	}
}

void USlotDataTask_Loader::Tick(float DeltaTime)
{
	if (bDeserializing)
	{
		if (CurrentLevel.IsValid())
			DeserializeASyncLoop();
	}
	else
	{
		// If Map load finished or didn't start and Data is loaded
		if (!bLoadingMap && IsDataLoaded())
			StartDeserialization();
	}
}

void USlotDataTask_Loader::OnFinish(bool bSuccess)
{
	if (bSuccess)
	{
		SELog(Preset, "Finished Loading", FColor::Green);
	}

	// Execute delegates
	Delegate.ExecuteIfBound((bSuccess) ? NewSlotInfo : nullptr);
	GetManager()->OnLoadFinished(Filter, !bSuccess);
}

void USlotDataTask_Loader::BeginDestroy()
{
	if (LoadDataTask) {
		LoadDataTask->EnsureCompletion(false);
		delete LoadDataTask;
	}

	Super::BeginDestroy();
}

void USlotDataTask_Loader::OnMapLoaded()
{
	const UWorld* World = GetWorld();
	if (World->GetFName() == NewSlotInfo->Map)
	{
		Filter.BakeAllowedClasses();
		bLoadingMap = false;

		if(IsDataLoaded())
			StartDeserialization();
	}
}

void USlotDataTask_Loader::StartDeserialization()
{
	check(NewSlotInfo);

	bDeserializing = true;
	NewSlotInfo->LoadDate = FDateTime::Now();

	USaveManager* Manager = GetManager();

	SlotData = GetLoadedData();
	if (!SlotData)
	{
		Finish(false);
		return;
	}

	Manager->OnLoadBegan(Filter);

	//Apply current Info if succeeded
	Manager->CurrentInfo = NewSlotInfo;

	BeforeDeserialize();

	if (Preset->IsFrameSplitLoad())
		DeserializeASync();
	else
		DeserializeSync();
}

void USlotDataTask_Loader::StartLoadingData()
{
	const FString SlotDataName = GetManager()->GenerateSlotDataName(Slot);
	LoadDataTask = new FAsyncTask<FLoadFileTask>(SlotDataName);

	if (Preset->IsMTFilesLoad())
		LoadDataTask->StartBackgroundTask();
	else
		LoadDataTask->StartSynchronousTask();
}

USlotData* USlotDataTask_Loader::GetLoadedData() const
{
	if (IsDataLoaded())
		return Cast<USlotData>(LoadDataTask->GetTask().GetSaveGame());
	return nullptr;
}

void USlotDataTask_Loader::BeforeDeserialize()
{
	UWorld* World = GetWorld();

	// Set current game time to the saved value
	World->TimeSeconds = SlotData->TimeSeconds;

	if (Preset->bStoreGameInstance)
		DeserializeGameInstance();
}

void USlotDataTask_Loader::DeserializeSync()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_Loading_DeserializeSync);

	const UWorld* World = GetWorld();
	check(World);

	SELog(Preset, "World '" + World->GetName() + "'", FColor::Green, false, 1);

	PrepareAllLevels();

	// Deserialize world
	{
		DeserializeLevelSync(World->GetCurrentLevel());

		const TArray<ULevelStreaming*>& Levels = World->GetStreamingLevels();
		for (const ULevelStreaming* Level : Levels)
		{
			if (Level->IsLevelLoaded())
			{
				DeserializeLevelSync(Level->GetLoadedLevel(), Level);
			}
		}
	}

	FinishedDeserializing();
}

void USlotDataTask_Loader::DeserializeLevelSync(const ULevel* Level, const ULevelStreaming* StreamingLevel)
{
	if (!IsValid(Level))
		return;

	QUICK_SCOPE_CYCLE_COUNTER(STAT_Loading_DeserializeLevelSync);

	const FName LevelName = StreamingLevel ? StreamingLevel->GetWorldAssetPackageFName() : FPersistentLevelRecord::PersistentName;
	SELog(Preset, "Level '" + LevelName.ToString() + "'", FColor::Green, false, 1);

	FLevelRecord* LevelRecord = FindLevelRecord(StreamingLevel);
	if (!LevelRecord)
		return;

	for (auto ActorItr = Level->Actors.CreateConstIterator(); ActorItr; ++ActorItr)
	{
		DeserializeLevel_Actor(*ActorItr, *LevelRecord);
	}
}

void USlotDataTask_Loader::DeserializeASync()
{
	// Deserialize world
	{
		SELog(Preset, "World '" + GetWorld()->GetName() + "'", FColor::Green, false, 1);

		PrepareAllLevels();

		DeserializeLevelASync(GetWorld()->GetCurrentLevel());
	}
}

void USlotDataTask_Loader::DeserializeLevelASync(ULevel* Level, ULevelStreaming* StreamingLevel)
{
	check(IsValid(Level));

	const FName LevelName = StreamingLevel ? StreamingLevel->GetWorldAssetPackageFName() : FPersistentLevelRecord::PersistentName;
	SELog(Preset, "Level '" + LevelName.ToString() + "'", FColor::Green, false, 1);

	FLevelRecord* LevelRecord = FindLevelRecord(StreamingLevel);
	if (!LevelRecord) {
		Finish(false);
		return;
	}

	const float StartMS = GetTimeMilliseconds();

	CurrentLevel = Level;
	CurrentSLevel = StreamingLevel;
	CurrentActorIndex = 0;

	// Copy actors array. New actors won't be considered for deserialization
	CurrentLevelActors.Empty(Level->Actors.Num());
	for (auto* Actor : Level->Actors)
	{
		if(IsValid(Actor))
			CurrentLevelActors.Add(Actor);
	}

	DeserializeASyncLoop(StartMS);
}

void USlotDataTask_Loader::DeserializeASyncLoop(float StartMS)
{
	FLevelRecord * LevelRecord = FindLevelRecord(CurrentSLevel.Get());
	if (!LevelRecord)
		return;

	if(StartMS <= 0)
		StartMS = GetTimeMilliseconds();

	// Continue Iterating actors every tick
	for (; CurrentActorIndex < CurrentLevelActors.Num(); ++CurrentActorIndex)
	{
		AActor* Actor{ CurrentLevelActors[CurrentActorIndex].Get() };
		if (Actor)
		{
			DeserializeLevel_Actor(Actor, *LevelRecord);

			const float CurrentMS = GetTimeMilliseconds();
			// If x milliseconds passed, continue on next frame
			if (CurrentMS - StartMS >= Filter.MaxFrameMs)
				return;
		}
	}

	ULevelStreaming* CurrentLevelStreaming = CurrentSLevel.Get();
	FindNextAsyncLevel(CurrentLevelStreaming);
	if (CurrentLevelStreaming)
	{
		// Iteration has ended. Deserialize next level
		CurrentLevel = CurrentLevelStreaming->GetLoadedLevel();
		if (CurrentLevel.IsValid())
		{
			DeserializeLevelASync(CurrentLevel.Get(), CurrentLevelStreaming);
			return;
		}
	}

	// All levels deserialized
	FinishedDeserializing();
}

void USlotDataTask_Loader::PrepareLevel(const ULevel* Level, const FLevelRecord& LevelRecord)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_Loading_PrepareLevel);

	// Records not contained in Scene Actors		 => Actors to be Respawned
	// Scene Actors not contained in loaded records  => Actors to be Destroyed
	// The rest									     => Just deserialize

	TArray<FActorRecord> ActorsToSpawn = LevelRecord.Actors;
	TArray<AActor*> ActorsToDestroy{};
	{
		// O(M*Log(N))
		for (auto ActorItr = Level->Actors.CreateConstIterator(); ActorItr; ++ActorItr)
		{
			AActor* const Actor{ *ActorItr };

			// Remove records which actors do exist
			const bool bFoundActorRecord = ActorsToSpawn.RemoveSingleSwap(Actor, false) > 0;

			if (Filter.ShouldSave(Actor))
			{
				if (!bFoundActorRecord) // Don't destroy level actors
				{
					// If the actor wasn't found, mark it for destruction
					Actor->Destroy();
				}
			}
		}
		ActorsToSpawn.Shrink();
	}

	// Create Actors that doesn't exist now but were saved
	RespawnActors(ActorsToSpawn, Level);
}

void USlotDataTask_Loader::FinishedDeserializing()
{
	// Clean serialization data
	SlotData->Clean(true);
	GetManager()->CurrentData = SlotData;

	Finish(true);
}

void USlotDataTask_Loader::PrepareAllLevels()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_Loading_PrepareAllLevels);

	const UWorld* World = GetWorld();
	check(World);

	// Prepare Main level
	PrepareLevel(World->GetCurrentLevel(), SlotData->MainLevel);

	// Prepare other loaded sub-levels
	const TArray<ULevelStreaming*>& Levels = World->GetStreamingLevels();
	for (const ULevelStreaming* Level : Levels)
	{
		if (Level->IsLevelLoaded())
		{
			const FLevelRecord* LevelRecord = FindLevelRecord(Level);
			if (LevelRecord)
			{
				PrepareLevel(Level->GetLoadedLevel(), *LevelRecord);
			}
		}
	}
}

void USlotDataTask_Loader::RespawnActors(const TArray<FActorRecord>& Records, const ULevel* Level)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_Loading_RespawnActors);

	FActorSpawnParameters SpawnInfo{};
	SpawnInfo.OverrideLevel = const_cast<ULevel*>(Level);

	UWorld* World = GetWorld();

	// Respawn all procedural actors
	for (const auto& Record : Records)
	{
		SpawnInfo.Name = Record.Name;
		World->SpawnActor(Record.Class, &Record.Transform, SpawnInfo);
	}
}

void USlotDataTask_Loader::DeserializeLevel_Actor(AActor* const Actor, const FLevelRecord& LevelRecord)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_Loading_DeserializeLevel_Actor);

	if (Filter.ShouldSave(Actor))
	{
		// Find the record
		const FActorRecord* const Record = LevelRecord.Actors.FindByKey(Actor);
		if (Record)
		{
			DeserializeActor(Actor, *Record);
		}
	}
}

void USlotDataTask_Loader::DeserializeGameInstance()
{
	bool bSuccess = true;
	auto* GameInstance = GetWorld()->GetGameInstance();
	const FObjectRecord& Record = SlotData->GameInstance;

	if (!IsValid(GameInstance) || GameInstance->GetClass() != Record.Class)
		bSuccess = false;

	if (bSuccess)
	{
		//Serialize from Record Data
		FMemoryReader MemoryReader(Record.Data, true);
		FSEArchive Archive(MemoryReader, false);
		GameInstance->Serialize(Archive);
	}

	SELog(Preset, "Game Instance '" + Record.Name.ToString() + "'", FColor::Green, !bSuccess, 1);
}

bool USlotDataTask_Loader::DeserializeActor(AActor* Actor, const FActorRecord& Record)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_Loading_DeserializeActor);

	if (!IsValid(Actor) || !Record.IsValid() || !Filter.ShouldSave(Actor) ||
		Actor->GetClass() != Record.Class)
	{
		return false;
	}

	// Always load saved tags
	Actor->Tags = Record.Tags;

	const bool bSavesPhysics = Filter.StoresPhysics(Actor);
	if (Filter.StoresTransform(Actor))
	{
		Actor->SetActorTransform(Record.Transform);

		if (Filter.StoresPhysics(Actor))
		{
			USceneComponent* Root = Actor->GetRootComponent();
			if (auto* Primitive = Cast<UPrimitiveComponent>(Root))
			{
				Primitive->SetPhysicsLinearVelocity(Record.LinearVelocity);
				Primitive->SetPhysicsAngularVelocityInRadians(Record.AngularVelocity);
			}
			else
			{
				Root->ComponentVelocity = Record.LinearVelocity;
			}
		}
	}

	Actor->SetActorHiddenInGame(Record.bHiddenInGame);

	DeserializeActorComponents(Actor, Record, 2);

	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_Loading_DataReader);
		//Serialize from Record Data
		FMemoryReader MemoryReader(Record.Data, true);
		FSEArchive Archive(MemoryReader, false);
		Actor->Serialize(Archive);
	}

	return true;
}

void USlotDataTask_Loader::DeserializeActorComponents(AActor* Actor, const FActorRecord& ActorRecord, int8 Indent)
{
	if (Filter.bStoreComponents)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_Loading_DeserializeActorComponents);

		const TSet<UActorComponent*>& Components = Actor->GetComponents();

		for (auto* Component : Components)
		{
			if (Filter.ShouldSave(Component))
			{
				// Find the record
				const FComponentRecord* Record = ActorRecord.ComponentRecords.FindByKey(Component);

				if (!Record) {
					SELog(Preset, "Component '" + Component->GetFName().ToString() + "' - Record not found", FColor::Red, false, Indent + 1);
					continue;
				}

				if (Filter.StoresTransform(Component))
				{
					USceneComponent* Scene = CastChecked<USceneComponent>(Component);
					if (Scene->Mobility == EComponentMobility::Movable)
					{
						Scene->SetRelativeTransform(Record->Transform);
					}
				}

				if (Filter.StoresTags(Component))
				{
					Component->ComponentTags = Record->Tags;
				}

				if (!Component->GetClass()->IsChildOf<UPrimitiveComponent>())
				{
					FMemoryReader MemoryReader(Record->Data, true);
					FSEArchive Archive(MemoryReader, false);
					Component->Serialize(Archive);
				}
			}
		}
	}
}

FLevelRecord* USlotDataTask_Loader::FindLevelRecord(const ULevelStreaming* Level) const
{
	if (!Level)
		return &SlotData->MainLevel;
	else // Find the Sub-Level
		return SlotData->SubLevels.FindByKey(Level);
}

void USlotDataTask_Loader::FindNextAsyncLevel(ULevelStreaming*& OutLevelStreaming) const
{
	OutLevelStreaming = nullptr;

	const UWorld* World = GetWorld();
	const TArray<ULevelStreaming*>& Levels = World->GetStreamingLevels();
	if (CurrentLevel.IsValid() && Levels.Num() > 0)
	{
		if (!CurrentSLevel.IsValid())
		{
			//Current is persistent, get first streaming level
			OutLevelStreaming = Levels[0];
			return;
		}
		else
		{
			int32 CurrentIndex = Levels.IndexOfByKey(CurrentSLevel);
			if (CurrentIndex != INDEX_NONE && Levels.Num() > CurrentIndex + 1)
			{
				OutLevelStreaming = Levels[CurrentIndex + 1];
			}
		}
	}

	// If this level is unloaded, find next
	if (OutLevelStreaming && !OutLevelStreaming->IsLevelLoaded())
		FindNextAsyncLevel(OutLevelStreaming);
}
