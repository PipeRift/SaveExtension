// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "Serialization/SlotDataTask_Loader.h"

#include <GameFramework/Character.h>
#include <Serialization/MemoryReader.h>
#include <Kismet/GameplayStatics.h>
#include <Components/PrimitiveComponent.h>
#include <UObject/UObjectGlobals.h>

#include "TimerManager.h"
#include "Misc/SlotHelpers.h"
#include "SavePreset.h"
#include "SaveManager.h"
#include "Serialization/SEArchive.h"


/////////////////////////////////////////////////////
// Helpers

namespace Loader
{
	static int32 RemoveSingleRecordPtrSwap(TArray<FActorRecord*>& Records, AActor* Actor, bool bAllowShrinking = true)
	{
		if(!Actor)
		{
			return 0;
		}

		const int32 I = Records.IndexOfByPredicate([Records, Actor](auto* Record)
		{
			return *Record == Actor;
		});
		if (I != INDEX_NONE)
		{
			Records.RemoveAtSwap(I, 1, bAllowShrinking);
			return 1;
		}
		return 0;
	}
}


/////////////////////////////////////////////////////
// USaveDataTask_Loader

void USlotDataTask_Loader::OnStart()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USlotDataTask_Loader::OnStart);
	USaveManager* Manager = GetManager();

	SELog(Preset, "Loading from Slot " + SlotName.ToString());

	NewSlotInfo = Manager->LoadInfo(SlotName);
	if (!NewSlotInfo)
	{
		SELog(Preset, "Slot Info not found! Can't load.", FColor::White, true, 1);
		Finish(false);
		return;
	}

	// We load data while the map opens or GC runs
	StartLoadingData();

	const UWorld* World = GetWorld();

	// Cross-Level loading
	// TODO: Handle empty Map as empty world
	FName CurrentMapName { FSlotHelpers::GetWorldName(World) };
	if (CurrentMapName != NewSlotInfo->Map)
	{
		LoadState = ELoadDataTaskState::LoadingMap;
		FString MapToOpen = NewSlotInfo->Map.ToString();
		if (!GEngine->MakeSureMapNameIsValid(MapToOpen))
		{
			UE_LOG(LogSaveExtension, Warning, TEXT("Slot '%s' was saved in map '%s' but it did not exist while loading. Corrupted save file?"), *NewSlotInfo->FileName.ToString(), *MapToOpen);
			Finish(false);
			return;
		}

		UGameplayStatics::OpenLevel(this, FName{ MapToOpen });

		SELog(Preset, "Slot '" + SlotName.ToString() + "' is recorded on another Map. Loading before charging slot.", FColor::White, false, 1);
		return;
	}
	else if (IsDataLoaded())
	{
		StartDeserialization();
	}
	else
	{
		LoadState = ELoadDataTaskState::WaitingForData;
	}
}

void USlotDataTask_Loader::Tick(float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USlotDataTask_Loader::Tick);
	switch(LoadState)
	{
	case ELoadDataTaskState::Deserializing:
		if (CurrentLevel.IsValid())
		{
			DeserializeASyncLoop();
		}
		break;

	case ELoadDataTaskState::WaitingForData:
		if (IsDataLoaded())
		{
			StartDeserialization();
		}
	}
}

void USlotDataTask_Loader::OnFinish(bool bSuccess)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USlotDataTask_Loader::OnFinish);
	if (bSuccess)
	{
		SELog(Preset, "Finished Loading", FColor::Green);
	}

	// Execute delegates
	Delegate.ExecuteIfBound((bSuccess) ? NewSlotInfo : nullptr);

	GetManager()->OnLoadFinished(
		SlotData? GetGeneralFilter() : FSELevelFilter{},
		!bSuccess
	);
}

void USlotDataTask_Loader::BeginDestroy()
{
	if (LoadDataTask)
	{
		LoadDataTask->EnsureCompletion(false);
		delete LoadDataTask;
	}

	Super::BeginDestroy();
}

void USlotDataTask_Loader::OnMapLoaded()
{
	if(LoadState != ELoadDataTaskState::LoadingMap)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if(!World)
	{
		UE_LOG(LogSaveExtension, Warning, TEXT("Failed loading map from saved slot."));
		Finish(false);
	}
	const FName NewMapName { FSlotHelpers::GetWorldName(World) };
	if (NewMapName == NewSlotInfo->Map)
	{
		if(IsDataLoaded())
		{
			StartDeserialization();
		}
		else
		{
			LoadState = ELoadDataTaskState::WaitingForData;
		}
	}
}

void USlotDataTask_Loader::StartDeserialization()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USlotDataTask_Loader::StartDeserialization);
	check(NewSlotInfo);

	LoadState = ELoadDataTaskState::Deserializing;

	SlotData = GetLoadedData();
	if (!SlotData)
	{
		// Failed to load data
		Finish(false);
		return;
	}

	NewSlotInfo->LoadDate = FDateTime::Now();

	GetManager()->OnLoadBegan(GetGeneralFilter());
	//Apply current Info if succeeded
	GetManager()->__SetCurrentInfo(NewSlotInfo);

	BakeAllFilters();

	BeforeDeserialize();

	if (Preset->IsFrameSplitLoad())
		DeserializeASync();
	else
		DeserializeSync();
}

void USlotDataTask_Loader::StartLoadingData()
{
	LoadDataTask = new FAsyncTask<FLoadFileTask>(GetManager(), SlotName.ToString());

	if (Preset->IsMTFilesLoad())
		LoadDataTask->StartBackgroundTask();
	else
		LoadDataTask->StartSynchronousTask();
}

USlotData* USlotDataTask_Loader::GetLoadedData() const
{
	if (IsDataLoaded())
	{
		return LoadDataTask->GetTask().GetData();
	}
	return nullptr;
}

void USlotDataTask_Loader::BeforeDeserialize()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USlotDataTask_Loader::BeforeDeserialize);
	UWorld* World = GetWorld();

	// Set current game time to the saved value
	World->TimeSeconds = SlotData->TimeSeconds;

	if (SlotData->bStoreGameInstance)
	{
		DeserializeGameInstance();
	}
}

void USlotDataTask_Loader::DeserializeSync()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USlotDataTask_Loader::DeserializeSync);

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
	TRACE_CPUPROFILER_EVENT_SCOPE(USlotDataTask_Loader::DeserializeLevelSync);

	if (!IsValid(Level))
		return;

	const FName LevelName = StreamingLevel ? StreamingLevel->GetWorldAssetPackageFName() : FPersistentLevelRecord::PersistentName;
	SELog(Preset, "Level '" + LevelName.ToString() + "'", FColor::Green, false, 1);

	if (FLevelRecord* LevelRecord = FindLevelRecord(StreamingLevel))
	{
		const auto& Filter = GetLevelFilter(*LevelRecord);

		for (auto ActorItr = Level->Actors.CreateConstIterator(); ActorItr; ++ActorItr)
		{
			auto* Actor = *ActorItr;
			if (IsValid(Actor) && Filter.ShouldSave(Actor))
			{
				DeserializeLevel_Actor(Actor, *LevelRecord, Filter);
			}
		}
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
		{
			CurrentLevelActors.Add(Actor);
		}
	}

	DeserializeASyncLoop(StartMS);
}

void USlotDataTask_Loader::DeserializeASyncLoop(float StartMS)
{
	FLevelRecord * LevelRecord = FindLevelRecord(CurrentSLevel.Get());
	if (!LevelRecord)
	{
		return;
	}

	const auto& Filter = GetLevelFilter(*LevelRecord);

	if(StartMS <= 0)
	{
		StartMS = GetTimeMilliseconds();
	}

	// Continue Iterating actors every tick
	for (; CurrentActorIndex < CurrentLevelActors.Num(); ++CurrentActorIndex)
	{
		AActor* const Actor{ CurrentLevelActors[CurrentActorIndex].Get() };
		if (IsValid(Actor) && Filter.ShouldSave(Actor))
		{
			DeserializeLevel_Actor(Actor, *LevelRecord, Filter);

			const float CurrentMS = GetTimeMilliseconds();
			// If x milliseconds passed, stop and continue on next frame
			if (CurrentMS - StartMS >= MaxFrameMs)
			{
				return;
			}
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

void USlotDataTask_Loader::PrepareLevel(const ULevel* Level, FLevelRecord& LevelRecord)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USlotDataTask_Loader::PrepareLevel);

	const auto& Filter = GetLevelFilter(LevelRecord);


	// Records not contained in Scene Actors		 => Actors to be Respawned
	// Scene Actors not contained in loaded records  => Actors to be Destroyed
	// The rest									     => Just deserialize

	TArray<FActorRecord*> ActorsToSpawn;
	ActorsToSpawn.Reserve(LevelRecord.Actors.Num());
	for(FActorRecord& Record : LevelRecord.Actors)
	{
		ActorsToSpawn.Add(&Record);
	}

	TArray<AActor*> ActorsToDestroy{};
	{
		// O(M*Log(N))
		for (AActor* const Actor : Level->Actors)
		{
			// Remove records which actors do exist
			const bool bFoundActorRecord = Loader::RemoveSingleRecordPtrSwap(ActorsToSpawn, Actor, false) > 0;

			if (Actor && Filter.ShouldSave(Actor))
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

	UWorld* const World = GetWorld();
	CollectGarbage(RF_NoFlags);
	FTimerHandle DummyHandle;
	// Collect garbage to avoid spawning issues
	World->GetTimerManager().SetTimer(
		DummyHandle,
		[this, ActorsToSpawn, Level]
		{
			// Create Actors that doesn't exist now but were saved
			this->RespawnActors(ActorsToSpawn, Level);
		},
		0.01f,
		false
	);
}

void USlotDataTask_Loader::FinishedDeserializing()
{
	// Clean serialization data
	SlotData->CleanRecords(true);
	GetManager()->__SetCurrentData(SlotData);

	Finish(true);
}

void USlotDataTask_Loader::PrepareAllLevels()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USlotDataTask_Loader::PrepareAllLevels);

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
			FLevelRecord* LevelRecord = FindLevelRecord(Level);
			if (LevelRecord)
			{
				PrepareLevel(Level->GetLoadedLevel(), *LevelRecord);
			}
		}
	}
}

// Function to find and return a UClass* from a class name string
UClass* USlotDataTask_Loader::GetClassFromName(const FString& ClassName) const
{
	if (ClassName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ClassName is empty."));
		return nullptr;
	}

	UClass* FoundClass = StaticLoadClass(UObject::StaticClass(), nullptr, *ClassName);
	return FoundClass;
}

void USlotDataTask_Loader::RespawnActors(const TArray<FActorRecord*>& Records, const ULevel* Level)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USlotDataTask_Loader::RespawnActors);

	FActorSpawnParameters SpawnInfo{};
	SpawnInfo.OverrideLevel = const_cast<ULevel*>(Level);
	SpawnInfo.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;

	UWorld* const World = GetWorld();

	// Respawn all procedural actors
	for (auto* Record : Records)
	{
		SpawnInfo.Name = Record->Name;

		UClass* RecordClass = Record->Class->IsValidLowLevel() ? Record->Class : GetClassFromName(Record->ClassName);
		if (RecordClass == nullptr || !RecordClass->IsValidLowLevel())
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to respawn actor."))
			continue;
		}

		auto* NewActor = World->SpawnActor(RecordClass, &Record->Transform, SpawnInfo);
		if (NewActor == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to respawn actor."))
			continue;
		}
		// We update the name on the record in case it changed
		Record->Name = NewActor->GetFName();
	}
}

void USlotDataTask_Loader::DeserializeLevel_Actor(AActor* const Actor, const FLevelRecord& LevelRecord, const FSELevelFilter& Filter)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USlotDataTask_Loader::DeserializeLevel_Actor);

	// Find the record
	const FActorRecord* const Record = LevelRecord.Actors.FindByKey(Actor);
	if (Record && Record->IsValid() && Record->Class == Actor->GetClass())
	{
		DeserializeActor(Actor, *Record, Filter);
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

bool USlotDataTask_Loader::DeserializeActor(AActor* Actor, const FActorRecord& Record, const FSELevelFilter& Filter)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USlotDataTask_Loader::DeserializeActor);

	// Always load saved tags
	Actor->Tags = Record.Tags;

	const bool bSavesPhysics = FSELevelFilter::StoresPhysics(Actor);
	if (FSELevelFilter::StoresTransform(Actor))
	{
		Actor->SetActorTransform(Record.Transform);

		if (FSELevelFilter::StoresPhysics(Actor))
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

	DeserializeActorComponents(Actor, Record, Filter, 2);

	{
		//Serialize from Record Data
		FMemoryReader MemoryReader(Record.Data, true);
		FSEArchive Archive(MemoryReader, false);
		Actor->Serialize(Archive);
	}

	return true;
}

void USlotDataTask_Loader::DeserializeActorComponents(AActor* Actor, const FActorRecord& ActorRecord, const FSELevelFilter& Filter, int8 Indent)
{
	if (Filter.bStoreComponents)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(UUSlotDataTask_Loader::DeserializeActorComponents);

		const TSet<UActorComponent*>& Components = Actor->GetComponents();
		for (auto* Component : Components)
		{
			if (!Filter.ShouldSave(Component))
			{
				continue;
			}

			// Find the record
			const FComponentRecord* Record = ActorRecord.ComponentRecords.FindByKey(Component);
			if (!Record)
			{
				SELog(Preset, "Component '" + Component->GetFName().ToString() + "' - Record not found", FColor::Red, false, Indent + 1);
				continue;
			}

			if (FSELevelFilter::StoresTransform(Component))
			{
				USceneComponent* Scene = CastChecked<USceneComponent>(Component);
				if (Scene->Mobility == EComponentMobility::Movable)
				{
					Scene->SetRelativeTransform(Record->Transform);
				}
			}

			if (FSELevelFilter::StoresTags(Component))
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
	{
		FindNextAsyncLevel(OutLevelStreaming);
	}
}
