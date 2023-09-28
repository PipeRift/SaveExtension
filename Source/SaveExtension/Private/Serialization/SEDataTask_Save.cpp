// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Serialization/SEDataTask_Save.h"

#include "Misc/SlotHelpers.h"
#include "SaveFileHelpers.h"
#include "SaveManager.h"
#include "SaveSlot.h"
#include "SaveSlotData.h"

#include <GameFramework/GameModeBase.h>
#include <Serialization/MemoryWriter.h>


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

		// Save Level info in both files
		Slot->Map = FName{FSlotHelpers::GetWorldName(World)};
		SlotData->Map = SlotData->Map;

		SlotData->bStoreGameInstance = Slot->bStoreGameInstance;

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

void FSEDataTask_Save::PrepareAllLevels(const TArray<ULevelStreaming*>& Levels)
{
	// Create the sub-level records if non existent
	for (const ULevelStreaming* Level : Levels)
	{
		if (Level->IsLevelLoaded())
		{
			SlotData->SubLevels.AddUnique({*Level});
		}
	}
}

void FSEDataTask_Save::SerializeLevelSync(
	const ULevel* Level, int32 AssignedTasks, const ULevelStreaming* StreamingLevel)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Save::SerializeLevelSync);
	check(IsValid(Level));

	if (!Slot->IsMTSerializationSave())
	{
		AssignedTasks = 1;
	}

	const FName LevelName =
		StreamingLevel ? StreamingLevel->GetWorldAssetPackageFName() : FPersistentLevelRecord::PersistentName;
	SELog(Slot, "Level '" + LevelName.ToString() + "'", FColor::Green, false, 1);

	// Find level record. By default, main level
	FLevelRecord* LevelRecord = &SlotData->RootLevel;
	if (StreamingLevel)
	{
		LevelRecord = FindLevelRecord(StreamingLevel);
	}
	check(LevelRecord);

	// Empty level record before serializing it
	LevelRecord->CleanRecords();

	const int32 MinObjectsPerTask = 40;
	const int32 ActorCount = Level->Actors.Num();
	const int32 NumBalancedPerTask = FMath::CeilToInt((float) ActorCount / AssignedTasks);
	const int32 NumPerTask = FMath::Max(NumBalancedPerTask, MinObjectsPerTask);

	// Split all actors between multi-threaded tasks
	int32 Index = 0;
	while (Index < ActorCount)
	{
		const int32 NumRemaining = ActorCount - Index;
		const int32 NumToSerialize = FMath::Min(NumRemaining, NumPerTask);

		// First task saves the GameInstance
		bool bStoreGameInstance = Index <= 0 && SlotData->bStoreGameInstance;
		// Add new Task
		Tasks.Emplace(FMTTask_SerializeActors{GetWorld(), SlotData, &Level->Actors, Index, NumToSerialize,
			bStoreGameInstance, LevelRecord, &LevelRecord->Filter});

		Index += NumToSerialize;
	}
}

void FSEDataTask_Save::RunScheduledTasks()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSEDataTask_Save::RunScheduledTasks);
	// Start all serialization tasks
	if (Tasks.Num() > 0)
	{
		for (int32 I = 1; I < Tasks.Num(); ++I)
		{
			if (Slot->IsMTSerializationSave())
				Tasks[I].StartBackgroundTask();
			else
				Tasks[I].StartSynchronousTask();
		}
		// First task stores
		Tasks[0].StartSynchronousTask();
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
