// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Serialization/SlotDataTask.h"

#include "SaveManager.h"


/////////////////////////////////////////////////////
// USaveDataTask

USaveSlotDataTask* USaveSlotDataTask::Start()
{
	const USaveManager* Manager = GetManager();

	// If not running and first task is this
	if (!bRunning && Manager->Tasks.Num() > 0 && Manager->Tasks[0] == this)
	{
		bRunning = true;
		OnStart();
	}
	return this;
}

void USaveSlotDataTask::Finish(bool bSuccess)
{
	if (bRunning)
	{
		OnFinish(bSuccess);
		MarkAsGarbage();
		GetManager()->FinishTask(this);
		bFinished = true;
		bSucceeded = bSuccess;
	}
}

bool USaveSlotDataTask::IsScheduled() const
{
	return GetManager()->Tasks.Contains(this);
}

USaveManager* USaveSlotDataTask::GetManager() const
{
	return Cast<USaveManager>(GetOuter());
}

void USaveSlotDataTask::BakeAllFilters()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USaveSlotDataTask::BakeAllFilters);
	SlotData->GlobalLevelFilter.BakeAllowedClasses();

	if (SlotData->MainLevel.bOverrideGlobalFilter)
	{
		SlotData->MainLevel.Filter.BakeAllowedClasses();
	}

	for (const auto& Level : SlotData->SubLevels)
	{
		if (Level.bOverrideGlobalFilter)
		{
			Level.Filter.BakeAllowedClasses();
		}
	}
}

const FSELevelFilter& USaveSlotDataTask::GetGlobalFilter() const
{
	check(SlotData);
	return SlotData->GlobalLevelFilter;
}

const FSELevelFilter& USaveSlotDataTask::GetLevelFilter(const FLevelRecord& Level) const
{
	if (Level.bOverrideGlobalFilter)
	{
		return Level.Filter;
	}
	return GetGlobalFilter();
}

FLevelRecord* USaveSlotDataTask::FindLevelRecord(const ULevelStreaming* Level) const
{
	if (!Level)
		return &SlotData->MainLevel;
	else	// Find the Sub-Level
		return SlotData->SubLevels.FindByKey(Level);
}

UWorld* USaveSlotDataTask::GetWorld() const
{
	return GetOuter()->GetWorld();
}
