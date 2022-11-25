// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "Serialization/SlotDataTask.h"

#include "SaveManager.h"
#include "SavePreset.h"


/////////////////////////////////////////////////////
// USaveDataTask

USlotDataTask* USlotDataTask::Start()
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

void USlotDataTask::Finish(bool bSuccess)
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

bool USlotDataTask::IsScheduled() const
{
	return GetManager()->Tasks.Contains(this);
}

USaveManager* USlotDataTask::GetManager() const
{
	return Cast<USaveManager>(GetOuter());
}

void USlotDataTask::BakeAllFilters()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USlotDataTask::BakeAllFilters);
	SlotData->GeneralLevelFilter.BakeAllowedClasses();

	if(SlotData->MainLevel.bOverrideGeneralFilter)
	{
		SlotData->MainLevel.Filter.BakeAllowedClasses();
	}

	for(const auto& Level : SlotData->SubLevels)
	{
		if(Level.bOverrideGeneralFilter)
		{
			Level.Filter.BakeAllowedClasses();
		}
	}
}

const FSELevelFilter& USlotDataTask::GetGeneralFilter() const
{
	check(SlotData);
	return SlotData->GeneralLevelFilter;
}

const FSELevelFilter& USlotDataTask::GetLevelFilter(const FLevelRecord& Level) const
{
	if(Level.bOverrideGeneralFilter)
	{
		return Level.Filter;
	}
	return GetGeneralFilter();
}

FLevelRecord* USlotDataTask::FindLevelRecord(const ULevelStreaming* Level) const
{
	if (!Level)
		return &SlotData->MainLevel;
	else // Find the Sub-Level
		return SlotData->SubLevels.FindByKey(Level);
}

UWorld* USlotDataTask::GetWorld() const
{
	return GetOuter()->GetWorld();
}
