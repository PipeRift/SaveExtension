// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Serialization/SEDataTask_Save.h"

#include "SaveExtension.h"
#include "SaveManager.h"
#include "SaveSlot.h"
#include "SaveSlotData.h"
#include "SEFileHelpers.h"
#include "Serialization/Records.h"
#include "Serialization/SEArchive.h"
#include "SEFileHelpers.h"

#include <GameFramework/GameModeBase.h>
#include <Serialization/MemoryWriter.h>
#include <Tasks/Task.h>


/////////////////////////////////////////////////////
// FSEDataTask_Save

FSEDataTask_Save::FSEDataTask_Save(USaveManager* Manager, USaveSlot* Slot)
	: FSEDataTask(Manager, ESETaskType::Save)
	, SlotData(Slot->AssureData())
{}

FSEDataTask_Save::~FSEDataTask_Save()
{
	if (!SaveFileTask.IsCompleted())
	{
		SaveFileTask.Wait();
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
		const bool bFileExists = FSEFileHelpers::FileExists(SlotNameStr);
		if (bOverride)
		{
			// Delete previous save
			if (bFileExists)
			{
				FSEFileHelpers::DeleteFile(SlotNameStr);
			}
		}
		else
		{
			// Only save if previous files don't exist
			// We don't want to serialize since it won't be saved anyway
			bSave = !bFileExists;
		}
	}

	if (!bSave)
	{
		Finish(false);
		return;
	}

	const UWorld* World = GetWorld();

	Manager->OnSaveBegan();

	check(SlotData->GetClass() == Slot->DataClass);
	SlotData->CleanRecords(true);

	check(Slot && SlotData);

	const bool bSlotExisted = Slot->Name == SlotName;
	Slot->Name = SlotName;

	if (bCaptureThumbnail)
	{
		bWaitingThumbnail = true;
		Slot->CaptureThumbnail(FSEOnThumbnailCaptured::CreateLambda([this](bool bSuccess) {
			bWaitingThumbnail = false;
		}), Width, Height);
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
	Slot->Map = FName{GetWorldName(World)};

	SerializeWorld();

	if (!bWaitingThumbnail) // Tick will check if thumbnail is not ready
	{
		SaveFile();
	}
}

void FSEDataTask_Save::Tick(float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Save::Tick);
	FSEDataTask::Tick(DeltaTime);

	if (SaveFileTask.IsValid() && SaveFileTask.IsCompleted())
	{
		Finish(SaveFileTask.GetResult());
	}
	else if (!SaveFileTask.IsValid() && !bWaitingThumbnail)
	{
		SaveFile();
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

	SubsystemFilter = Slot->SubsystemFilter;
	SubsystemFilter.BakeAllowedClasses();

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

			SlotData->GameInstanceSubsystems.Reset();
			for(UGameInstanceSubsystem* Subsystem : GameInstance->GetSubsystemArray<UGameInstanceSubsystem>())
			{
				if (SubsystemFilter.IsAllowed(Subsystem->GetClass()))
				{
					auto& SubsystemRecord = SlotData->GameInstanceSubsystems.Add_GetRef({Subsystem});
					FMemoryWriter SubsystemMemoryWriter(SubsystemRecord.Data, true);
					FSEArchive Ar(SubsystemMemoryWriter, false);
					Subsystem->Serialize(Ar);
				}
			}
		}

		SlotData->WorldSubsystems.Reset();
		for(UWorldSubsystem* Subsystem : World->GetSubsystemArray<UWorldSubsystem>())
		{
			if (SubsystemFilter.IsAllowed(Subsystem->GetClass()))
			{
				auto& SubsystemRecord = SlotData->WorldSubsystems.Add_GetRef({Subsystem});
				FMemoryWriter SubsystemMemoryWriter(SubsystemRecord.Data, true);
				FSEArchive Ar(SubsystemMemoryWriter, false);
				Subsystem->Serialize(Ar);
			}
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
	auto& LevelRecord = StreamingLevel? *FindLevelRecord(*SlotData, StreamingLevel) : SlotData->RootLevel;
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
		SERecords::SerializeActor(ActorsToSerialize[i], LevelRecord.Actors[i], Filter.ComponentFilter);
	}, Slot->ShouldSerializeAsync()? EParallelForFlags::None : EParallelForFlags::ForceSingleThread);
}

void FSEDataTask_Save::SaveFile()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Save::SaveFile);
	SaveFileTask = FSEFileHelpers::SaveFile(Manager->GetActiveSlot(), SlotName.ToString(), Slot->bUseCompression);

	if (!Slot->ShouldSaveFileAsync())
	{
		SaveFileTask.Wait();
		Finish(SaveFileTask.GetResult());
	}
}
