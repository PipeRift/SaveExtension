// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Serialization/SEDataTask_Save.h"

#include "Misc/SlotHelpers.h"
#include "SaveFileHelpers.h"
#include "SaveManager.h"
#include "SaveSlot.h"
#include "SaveSlotData.h"

#include <GameFramework/GameModeBase.h>
#include <Serialization/MemoryWriter.h>
#include <Tasks/Task.h>


void SerializeActorComponents(
	const AActor* Actor, FActorRecord& ActorRecord, const FSELevelFilter& Filter)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SerializeActorComponents);

	const TSet<UActorComponent*>& Components = Actor->GetComponents();
	for (auto* Component : Components)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(SerializeActorComponents | Component);
		if (IsValid(Component) && Filter.Stores(Component))
		{
			FComponentRecord& ComponentRecord = ActorRecord.ComponentRecords.Add_GetRef({Component});

			if (Filter.StoresTransform(Component))
			{
				const USceneComponent* Scene = CastChecked<USceneComponent>(Component);
				if (Scene->Mobility == EComponentMobility::Movable)
				{
					ComponentRecord.Transform = Scene->GetRelativeTransform();
				}
			}

			if (Filter.StoresTags(Component))
			{
				ComponentRecord.Tags = Component->ComponentTags;
			}

			if (!Component->GetClass()->IsChildOf<UPrimitiveComponent>())
			{
				FMemoryWriter MemoryWriter(ComponentRecord.Data, true);
				FSEArchive Archive(MemoryWriter, false);
				Component->Serialize(Archive);
			}
		}
	}
}

bool SerializeActor(const AActor* Actor, FActorRecord& Record, const FSELevelFilter& Filter)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SerializeActor);

	Record = FActorRecord{Actor};

	Record.bHiddenInGame = Actor->IsHidden();
	Record.bIsProcedural = Filter.IsProcedural(Actor);

	if (Filter.StoresTags(Actor))
	{
		Record.Tags = Actor->Tags;
	}
	else
	{
		// Only save save-tags
		for (const auto& Tag : Actor->Tags)
		{
			if (Filter.IsSaveTag(Tag))
			{
				Record.Tags.Add(Tag);
			}
		}
	}

	if (Filter.StoresTransform(Actor))
	{
		Record.Transform = Actor->GetTransform();

		if (Filter.StoresPhysics(Actor))
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

	if (Filter.StoresAnyComponents())
	{
		SerializeActorComponents(Actor, Record, Filter);
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(SerializeActor | Serialize);
	FMemoryWriter MemoryWriter(Record.Data, true);
	FSEArchive Archive(MemoryWriter, false);
	const_cast<AActor*>(Actor)->Serialize(Archive);
	return true;
}


/////////////////////////////////////////////////////
// FSEDataTask_Save

FSEDataTask_Save::~FSEDataTask_Save()
{
	if (SaveTask)
	{
		SaveTask->EnsureCompletion(false);
		delete SaveTask;
	}
}

void FSEDataTask_Save::OnStart()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Save::OnStart);
	Manager->AssureActiveSlot();

	bool bSave = true;
	const FString SlotNameStr = SlotName.ToString();
	// Overriding
	{
		const bool bFileExists = FSaveFileHelpers::FileExists(SlotNameStr);
		if (bOverride)
		{
			// Delete previous save
			if (bFileExists)
			{
				FSaveFileHelpers::DeleteFile(SlotNameStr);
			}
		}
		else
		{
			// Only save if previous files don't exist
			// We don't want to serialize since it won't be saved anyway
			bSave = !bFileExists;
		}
	}

	if (bSave)
	{
		const UWorld* World = GetWorld();

		Manager->OnSaveBegan();

		Slot = Manager->GetActiveSlot();
		SlotData = Slot->GetData();
		check(SlotData->GetClass() == Slot->DataClass);
		SlotData->CleanRecords(true);

		check(Slot && SlotData);

		const bool bSlotExisted = Slot->FileName == SlotName;
		Slot->FileName = SlotName;

		if (bSaveThumbnail)
		{
			Slot->CaptureThumbnail(Width, Height);
		}

		// Time stats
		{
			FSaveSlotStats& Stats = Slot->Stats;
			Stats.SaveDate = FDateTime::Now();

			// If this info has been loaded ever
			const bool bWasLoaded = Stats.LoadDate.GetTicks() > 0;
			if (bWasLoaded)
			{
				// Now - Loaded
				const FTimespan SessionTime = Stats.SaveDate - Stats.LoadDate;
				Stats.PlayedTime += SessionTime;
				Stats.SlotPlayedTime = bSlotExisted ? (Stats.SlotPlayedTime + SessionTime) : SessionTime;
			}
			else
			{
				// Slot is new, played time is world seconds
				Stats.PlayedTime = FTimespan::FromSeconds(World->TimeSeconds);
				Stats.SlotPlayedTime = Stats.PlayedTime;
			}

			// Save current game seconds
			SlotData->TimeSeconds = World->TimeSeconds;
		}

		// Save Level info
		Slot->Map = FName{FSlotHelpers::GetWorldName(World)};

		SerializeWorld();
		SaveFile();
		return;
	}
	Finish(false);
}

void FSEDataTask_Save::Tick(float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Save::Tick);
	FSEDataTask::Tick(DeltaTime);

	if (SaveTask && SaveTask->IsDone())
	{
		if (bSaveThumbnail)
		{
			if (Slot && Slot->GetThumbnail())
			{
				Finish(true);
			}
		}
		else
		{
			Finish(true);
		}
	}
}

void FSEDataTask_Save::OnFinish(bool bSuccess)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Save::OnFinish);
	if (bSuccess)
	{
		// Clean serialization data
		SlotData->CleanRecords(true);

		SELog(Slot, "Finished Saving", FColor::Green);
	}

	// Execute delegates
	Delegate.ExecuteIfBound((Manager && bSuccess) ? Manager->GetActiveSlot() : nullptr);

	Manager->OnSaveFinished(!bSuccess);
}

void FSEDataTask_Save::SerializeWorld()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Save::SerializeWorld);

	// Must have Authority
	const UWorld* World = GetWorld();
	if (!World->GetAuthGameMode())
	{
		return;
	}

	SELog(Slot, "World '" + World->GetName() + "'", FColor::Green, false, 1);

	const TArray<ULevelStreaming*>& Levels = World->GetStreamingLevels();
	PrepareAllLevels(Levels);

	{ // Serialization
		UGameInstance* GameInstance = World->GetGameInstance();
		if (GameInstance && Slot->bStoreGameInstance)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(SerializeGameInstance);
			FObjectRecord Record{GameInstance};
			FMemoryWriter MemoryWriter(Record.Data, true);
			FSEArchive Archive(MemoryWriter, false);
			GameInstance->Serialize(Archive);
			SlotData->GameInstance = MoveTemp(Record);
		}

		SerializeLevel(World->GetCurrentLevel());
		for (const ULevelStreaming* Level : Levels)
		{
			if (Level->IsLevelLoaded())
			{
				SerializeLevel(Level->GetLoadedLevel(), Level);
			}
		}
	}
}

void FSEDataTask_Save::PrepareAllLevels(const TArray<ULevelStreaming*>& Levels)
{
	// Prepare root level
	PrepareLevel(GetWorld()->GetCurrentLevel(), SlotData->RootLevel);

	// Create the sub-level records if non existent
	for (const ULevelStreaming* Level : Levels)
	{
		if (Level->IsLevelLoaded())
		{
			FLevelRecord& LevelRecord = SlotData->SubLevels.Add_GetRef({*Level});
			PrepareLevel(Level->GetLoadedLevel(), LevelRecord);
		}
	}
}

void FSEDataTask_Save::PrepareLevel(const ULevel* Level, FLevelRecord& LevelRecord)
{
	Slot->GetLevelFilter(true, LevelRecord.Filter);
	LevelRecord.Filter.BakeAllowedClasses();
}

void FSEDataTask_Save::SerializeLevel(
	const ULevel* Level, const ULevelStreaming* StreamingLevel)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Save::SerializeLevel);
	check(IsValid(Level));

	const FName LevelName =
		StreamingLevel ? StreamingLevel->GetWorldAssetPackageFName() : FPersistentLevelRecord::PersistentName;
	SELog(Slot, "Level '" + LevelName.ToString() + "'", FColor::Green, false, 1);

	// Find level record. By default, main level
	auto& LevelRecord = StreamingLevel? *FindLevelRecord(StreamingLevel) : SlotData->RootLevel;
	const FSELevelFilter& Filter = LevelRecord.Filter;

	LevelRecord.CleanRecords(); // Empty level record before serializing it

	TArray<const AActor*> ActorsToSerialize;
	for (AActor* Actor : Level->Actors)
	{
		if (Actor && Filter.Stores(Actor))
		{
			ActorsToSerialize.Add(Actor);
		}
	}
	LevelRecord.Actors.SetNum(ActorsToSerialize.Num());

	ParallelFor(ActorsToSerialize.Num(), [&LevelRecord, &ActorsToSerialize, &Filter](int32 i)
	{
		SerializeActor(ActorsToSerialize[i], LevelRecord.Actors[i], Filter);
	}, Slot->ShouldSerializeAsync()? EParallelForFlags::None : EParallelForFlags::ForceSingleThread);
}

void FSEDataTask_Save::SaveFile()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Save::SaveFile);
	SaveTask =
		new FAsyncTask<FSaveFileTask>(Manager->GetActiveSlot(), SlotName.ToString(), Slot->bUseCompression);

	if (Slot->IsMTFilesSave())
	{
		SaveTask->StartBackgroundTask();
	}
	else
	{
		SaveTask->StartSynchronousTask();

		if (!bSaveThumbnail)
		{
			Finish(true);
		}
	}
}
