// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Serialization/SEDataTask.h"

#include "SaveManager.h"


/////////////////////////////////////////////////////
// USaveDataTask

FSEDataTask& FSEDataTask::Start()
{
	// If not running and first task is this
	if (!bRunning && Manager->Tasks.Num() > 0 && Manager->Tasks[0].Get() == this)
	{
		bRunning = true;
		OnStart();
	}
	return *this;
}

void FSEDataTask::Finish(bool bSuccess)
{
	if (bRunning)
	{
		OnFinish(bSuccess);
		Manager->FinishTask(this);
		bFinished = true;
		bSucceeded = bSuccess;
	}
}

bool FSEDataTask::IsScheduled() const
{
	return Manager->Tasks.ContainsByPredicate([this](auto& Task) {
		return Task.Get() == this;
	});
}

FLevelRecord* FSEDataTask::FindLevelRecord(const ULevelStreaming* Level) const
{
	if (Level)
	{
		return SlotData->SubLevels.FindByKey(Level);
	}
	return &SlotData->RootLevel;
}

UWorld* FSEDataTask::GetWorld() const
{
	return Manager->GetWorld();
}

FString FSEDataTask::GetWorldName(const UWorld* World)
{
	check(World);
	const FString MapName = World->GetOutermost()->GetName();
	if (World->IsPlayInEditor())
	{
		return UWorld::RemovePIEPrefix(MapName);
	}
	return MapName;
}