// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Serialization/SEDataTask_Load.h"

#include "SEFileHelpers.h"
#include "SaveExtension.h"
#include "SaveManager.h"
#include "SaveSlot.h"
#include "SaveSlotData.h"
#include "Serialization/Records.h"
#include "Serialization/SEArchive.h"

#include <Components/PrimitiveComponent.h>
#include <GameFramework/Character.h>
#include <Kismet/GameplayStatics.h>
#include <Serialization/MemoryReader.h>
#include <UObject/UObjectGlobals.h>


/////////////////////////////////////////////////////
// USaveDataTask_Loader

FSEDataTask_Load::FSEDataTask_Load(USaveManager* Manager, USaveSlot* Slot)
	: FSEDataTask(Manager, ESETaskType::Load)
	, SlotData(Slot->GetData())
	, MaxFrameMs(Slot->GetMaxFrameMs())
{}

FSEDataTask_Load::~FSEDataTask_Load()
{
	if (!LoadFileTask.IsCompleted())
	{
		LoadFileTask.Wait();
	}
}

void FSEDataTask_Load::OnStart()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Load::OnStart);

	Slot = Manager->PreloadSlot(SlotName);
	SELog(Slot, "Loading from Slot " + SlotName.ToString());
	if (!Slot)
	{
		SELog(Slot, "Slot Info not found! Can't load.", FColor::White, true, 1);
		Finish(false);
		return;
	}

	// We load data while the map opens or GC runs
	StartLoadingFile();

	const UWorld* World = GetWorld();

	// Cross-Level loading
	// TODO: Handle empty Map as empty world
	FName CurrentMapName{GetWorldName(World)};
	if (CurrentMapName != Slot->Map)
	{
		LoadState = ELoadDataTaskState::LoadingMap;
		FString MapToOpen = Slot->Map.ToString();
		if (!GEngine->MakeSureMapNameIsValid(MapToOpen))
		{
			UE_LOG(LogSaveExtension, Warning,
				TEXT("Slot '%s' was saved in map '%s' but it did not exist while loading. Corrupted save "
					 "file?"),
				*Slot->Name.ToString(), *MapToOpen);
			Finish(false);
			return;
		}

		UGameplayStatics::OpenLevel(Manager, FName{MapToOpen});

		SELog(Slot,
			"Slot '" + SlotName.ToString() + "' is recorded on another Map. Loading before charging slot.",
			FColor::White, false, 1);
		return;
	}
	else if (CheckFileLoaded())
	{
		StartDeserialization();
	}
	else
	{
		LoadState = ELoadDataTaskState::WaitingForData;
	}
}

void FSEDataTask_Load::Tick(float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Load::Tick);
	switch (LoadState)
	{
		case ELoadDataTaskState::Deserializing:
			if (CurrentLevel.IsValid())
			{
				DeserializeASyncLoop();
			}
			break;

		case ELoadDataTaskState::WaitingForData:
			if (CheckFileLoaded())
			{
				StartDeserialization();
			}
	}
}

void FSEDataTask_Load::OnFinish(bool bSuccess)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Load::OnFinish);
	if (bSuccess)
	{
		SELog(Slot, "Finished Loading", FColor::Green);
	}

	// Execute delegates
	Delegate.ExecuteIfBound((bSuccess) ? Slot : nullptr);

	Manager->OnLoadFinished(!bSuccess);
}

void FSEDataTask_Load::OnMapLoaded()
{
	if (LoadState != ELoadDataTaskState::LoadingMap)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogSaveExtension, Warning, TEXT("Failed loading map from saved slot."));
		Finish(false);
	}
	const FName NewMapName{GetWorldName(World)};
	if (NewMapName == Slot->Map)
	{
		if (CheckFileLoaded())
		{
			StartDeserialization();
		}
		else
		{
			LoadState = ELoadDataTaskState::WaitingForData;
		}
	}
}

void FSEDataTask_Load::StartDeserialization()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Load::StartDeserialization);
	check(Slot);

	LoadState = ELoadDataTaskState::Deserializing;

	if (!SlotData)
	{
		// Failed to load data
		Finish(false);
		return;
	}

	Slot->Stats.LoadDate = FDateTime::Now();

	// Apply current Info if succeeded
	Manager->SetActiveSlot(Slot);

	Manager->OnLoadBegan();

	BeforeDeserialize();

	if (Slot->IsFrameSplitLoad())
		DeserializeASync();
	else
		DeserializeSync();
}

void FSEDataTask_Load::StartLoadingFile()
{
	LoadFileTask = FSEFileHelpers::LoadFile(SlotName.ToString(), Slot, true, Manager);
	if (!Slot->ShouldLoadFileAsync())
	{
		LoadFileTask.Wait();
		CheckFileLoaded();
	}
}

bool FSEDataTask_Load::CheckFileLoaded()
{
	if (LoadFileTask.IsCompleted())
	{
		Slot = LoadFileTask.GetResult();
		SlotData = Slot->GetData();
		return true;
	}
	return false;
}

void FSEDataTask_Load::BeforeDeserialize()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Load::BeforeDeserialize);

	SubsystemFilter = Slot->SubsystemFilter;
	SubsystemFilter.BakeAllowedClasses();

	UWorld* World = GetWorld();

	// Set current game time to the saved value
	World->TimeSeconds = SlotData->TimeSeconds;

	auto* GameInstance = GetWorld()->GetGameInstance();
	if (IsValid(GameInstance) && Slot->bStoreGameInstance)
	{
		if (GameInstance->GetClass() == SlotData->GameInstance.Class)
		{
			// Serialize from Record Data
			FMemoryReader MemoryReader(SlotData->GameInstance.Data, true);
			FSEArchive Archive(MemoryReader, false);
			GameInstance->Serialize(Archive);
		}

		for (const FSubsystemRecord& SubsystemRecord : SlotData->GameInstanceSubsystems)
		{
			if (SubsystemRecord.IsValid() && SubsystemFilter.IsAllowed(SubsystemRecord.Class))
			{
				if (USubsystem* Subsystem = GameInstance->GetSubsystemBase(SubsystemRecord.Class))
				{
					FMemoryReader SubsystemMemoryReader(SubsystemRecord.Data, true);
					FSEArchive Ar(SubsystemMemoryReader, false);
					Subsystem->Serialize(Ar);
				}
			}
		}
	}

	for (const FSubsystemRecord& SubsystemRecord : SlotData->WorldSubsystems)
	{
		if (SubsystemRecord.IsValid() && SubsystemFilter.IsAllowed(SubsystemRecord.Class))
		{
			if (USubsystem* Subsystem = World->GetSubsystemBase(SubsystemRecord.Class))
			{
				FMemoryReader SubsystemMemoryReader(SubsystemRecord.Data, true);
				FSEArchive Ar(SubsystemMemoryReader, false);
				Subsystem->Serialize(Ar);
			}
		}
	}
}

void FSEDataTask_Load::DeserializeSync()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Load::DeserializeSync);

	const UWorld* World = GetWorld();
	check(World);

	SELog(Slot, "World '" + World->GetName() + "'", FColor::Green, false, 1);

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

void FSEDataTask_Load::DeserializeLevelSync(const ULevel* Level, const ULevelStreaming* StreamingLevel)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Load::DeserializeLevelSync);

	if (!IsValid(Level))
	{
		return;
	}

	const FName LevelName =
		StreamingLevel ? StreamingLevel->GetWorldAssetPackageFName() : FPersistentLevelRecord::PersistentName;
	SELog(Slot, "Level '" + LevelName.ToString() + "'", FColor::Green, false, 1);

	const FLevelRecord& LevelRecord = *FindLevelRecord(*SlotData, StreamingLevel);
	for (const auto& RecordToActor : LevelRecord.RecordsToActors)
	{
		const FActorRecord* Record = RecordToActor.Key;
		AActor* Actor = RecordToActor.Value.Get();
		check(Record && Actor);
		DeserializeActor(Actor, *Record, LevelRecord);
	}
}

void FSEDataTask_Load::DeserializeASync()
{
	// Deserialize world
	{
		SELog(Slot, "World '" + GetWorld()->GetName() + "'", FColor::Green, false, 1);

		PrepareAllLevels();
		DeserializeLevelASync(GetWorld()->GetCurrentLevel());
	}
}

void FSEDataTask_Load::DeserializeLevelASync(ULevel* Level, ULevelStreaming* StreamingLevel)
{
	check(IsValid(Level));

	const FName LevelName =
		StreamingLevel ? StreamingLevel->GetWorldAssetPackageFName() : FPersistentLevelRecord::PersistentName;
	SELog(Slot, "Level '" + LevelName.ToString() + "'", FColor::Green, false, 1);

	FLevelRecord* LevelRecord = FindLevelRecord(*SlotData, StreamingLevel);
	if (!LevelRecord)
	{
		Finish(false);
		return;
	}

	const float StartMS = GetTimeMilliseconds();

	CurrentLevel = Level;
	CurrentSLevel = StreamingLevel;
	CurrentActorIndex = 0;

	DeserializeASyncLoop(StartMS);
}

void FSEDataTask_Load::DeserializeASyncLoop(float StartMS)
{
	if (StartMS <= 0)
	{
		StartMS = GetTimeMilliseconds();
	}

	FLevelRecord& LevelRecord = *FindLevelRecord(*SlotData, CurrentSLevel.Get());

	// Continue Iterating actors every tick
	for (; CurrentActorIndex < LevelRecord.RecordsToActors.Num(); ++CurrentActorIndex)
	{
		auto& RecordToActor = LevelRecord.RecordsToActors[CurrentActorIndex];

		const FActorRecord* Record = RecordToActor.Key;
		AActor* Actor = RecordToActor.Value.Get();
		check(Record);
		if (!Actor)
		{
			continue;
		}
		DeserializeActor(Actor, *Record, LevelRecord);

		const float CurrentMS = GetTimeMilliseconds();
		if (CurrentMS - StartMS >= MaxFrameMs)
		{
			// If x milliseconds passed, stop and continue on next frame
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

void FSEDataTask_Load::PrepareLevel(const ULevel* Level, FLevelRecord& LevelRecord)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Load::PrepareLevel);

	Slot->GetLevelFilter(true, LevelRecord.Filter);
	LevelRecord.Filter.BakeAllowedClasses();

	// Records not contained in Scene Actors		 => Actors to be Respawned
	// Scene Actors not contained in loaded records  => Actors to be Destroyed
	// The rest									     => Just deserialize

	TArray<FActorRecord*> ActorRecordsToSpawn;
	ActorRecordsToSpawn.Reserve(LevelRecord.Actors.Num());
	for (FActorRecord& Record : LevelRecord.Actors)
	{
		ActorRecordsToSpawn.Add(&Record);
	}

	TArray<AActor*> ActorsToDestroy{};
	{	 // Filter actors by whether they should be destroyed or spawned - O(M*Log(N))
		for (AActor* const Actor : Level->Actors)
		{
			if (UNLIKELY(!Actor))
			{
				continue;
			}

			const int32 Index = ActorRecordsToSpawn.IndexOfByPredicate([Actor](auto* Record) {
				return *Record == Actor;
			});
			if (Index != INDEX_NONE)	// Actor found, therefore doesn't need to be spawned
			{
				if (LevelRecord.Filter.Stores(Actor))
				{
					FActorRecord* Record = ActorRecordsToSpawn[Index];
					LevelRecord.RecordsToActors.Add({Record, Actor});
				}
				ActorRecordsToSpawn.RemoveAtSwap(Index, 1, false);
			}
			else if (LevelRecord.Filter.Stores(Actor))
			{
				ActorsToDestroy.Add(Actor);
			}
			// TODO: Consider unmatching class actors to be respawned
		}
	}

	// The serializable actors that were not found will be destroyed
	for (AActor* Actor : ActorsToDestroy)
	{
		Actor->Destroy();
	}

	// Spawn Actors that don't exist but were saved
	ActorRecordsToSpawn.Shrink();
	RespawnActors(ActorRecordsToSpawn, Level, LevelRecord);
}

void FSEDataTask_Load::FinishedDeserializing()
{
	// Clean serialization data
	SlotData->CleanRecords(true);
	Slot->AssignData(SlotData);
	Finish(true);
}

void FSEDataTask_Load::PrepareAllLevels()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Load::PrepareAllLevels);

	const UWorld* World = GetWorld();
	check(World);

	// Prepare root level
	PrepareLevel(World->GetCurrentLevel(), SlotData->RootLevel);

	// Prepare other loaded sub-levels
	const TArray<ULevelStreaming*>& Levels = World->GetStreamingLevels();
	for (const ULevelStreaming* Level : Levels)
	{
		if (Level->IsLevelLoaded())
		{
			FLevelRecord* LevelRecord = FindLevelRecord(*SlotData, Level);
			if (LevelRecord)
			{
				PrepareLevel(Level->GetLoadedLevel(), *LevelRecord);
			}
		}
	}
}

void FSEDataTask_Load::RespawnActors(
	const TArray<FActorRecord*>& Records, const ULevel* Level, FLevelRecord& LevelRecord)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Load::RespawnActors);

	FActorSpawnParameters SpawnInfo{};
	SpawnInfo.OverrideLevel = const_cast<ULevel*>(Level);
	SpawnInfo.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;

	UWorld* const World = GetWorld();

	// Respawn all procedural actors
	for (auto* Record : Records)
	{
		SpawnInfo.Name = Record->Name;
		auto* NewActor = World->SpawnActor(Record->Class, &Record->Transform, SpawnInfo);

		// We update the name on the record in case it changed
		Record->Name = NewActor->GetFName();
		LevelRecord.RecordsToActors.Add({Record, NewActor});
	}
}

bool FSEDataTask_Load::DeserializeActor(
	AActor* Actor, const FActorRecord& ActorRecord, const FLevelRecord& LevelRecord)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Load::DeserializeActor);

	if (Actor->GetClass() != ActorRecord.Class)
	{
		SELog(Slot, "Actor '" + ActorRecord.Name.ToString() + "' already exists but class doesn't match",
			FColor::Green, true, 1);
		return false;
	}

	// Always load saved tags
	Actor->Tags = ActorRecord.Tags;

	const bool bSavesPhysics = FSELevelFilter::StoresPhysics(Actor);
	if (FSELevelFilter::StoresTransform(Actor))
	{
		Actor->SetActorTransform(ActorRecord.Transform);

		if (FSELevelFilter::StoresPhysics(Actor))
		{
			USceneComponent* Root = Actor->GetRootComponent();
			if (auto* Primitive = Cast<UPrimitiveComponent>(Root))
			{
				Primitive->SetPhysicsLinearVelocity(ActorRecord.LinearVelocity);
				Primitive->SetPhysicsAngularVelocityInRadians(ActorRecord.AngularVelocity);
			}
			else
			{
				Root->ComponentVelocity = ActorRecord.LinearVelocity;
			}
		}
	}

	Actor->SetActorHiddenInGame(ActorRecord.bHiddenInGame);

	DeserializeActorComponents(Actor, ActorRecord, LevelRecord, 2);

	{
		// Serialize from Record Data
		FMemoryReader MemoryReader(ActorRecord.Data, true);
		FSEArchive Archive(MemoryReader, false);
		Actor->Serialize(Archive);
	}

	return true;
}

void FSEDataTask_Load::DeserializeActorComponents(
	AActor* Actor, const FActorRecord& ActorRecord, const FLevelRecord& LevelRecord, int8 Indent)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UFSEDataTask_Load::DeserializeActorComponents);

	if (!LevelRecord.Filter.StoresAnyComponents())
	{
		return;
	}

	for (auto* Component : Actor->GetComponents())
	{
		if (!IsValid(Component) || !LevelRecord.Filter.Stores(Component))
		{
			continue;
		}

		// Find the record
		const FComponentRecord* Record = ActorRecord.ComponentRecords.FindByKey(Component);
		if (!Record)
		{
			SELog(Slot, "Component '" + Component->GetFName().ToString() + "' - Record not found",
				FColor::Red, false, Indent + 1);
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

void FSEDataTask_Load::FindNextAsyncLevel(ULevelStreaming*& OutLevelStreaming) const
{
	OutLevelStreaming = nullptr;

	const UWorld* World = GetWorld();
	const TArray<ULevelStreaming*>& Levels = World->GetStreamingLevels();
	if (CurrentLevel.IsValid() && Levels.Num() > 0)
	{
		if (!CurrentSLevel.IsValid())
		{
			// Current is persistent, get first streaming level
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
