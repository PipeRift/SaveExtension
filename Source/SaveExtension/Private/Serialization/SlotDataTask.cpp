// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "Serialization/SlotDataTask.h"

#include "SaveManager.h"
#include "SavePreset.h"


/////////////////////////////////////////////////////
// USaveDataTask

const FName USlotDataTask::TagNoSave{ "!Save" };
const FName USlotDataTask::TagNoTransform{ "!SaveTransform" };
const FName USlotDataTask::TagNoPhysics{ "!SavePhysics" };
const FName USlotDataTask::TagNoComponents{ "!SaveComponents" };
const FName USlotDataTask::TagNoTags{ "!SaveTags" };
const FName USlotDataTask::TagTransform{ "SaveTransform" };


bool USlotDataTask::IsSaveTag(const FName& Tag)
{
	return Tag == TagNoSave || Tag == TagNoTransform || Tag == TagNoPhysics || Tag == TagNoComponents || Tag == TagNoTags;
}

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
		MarkPendingKill();
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

UWorld* USlotDataTask::GetWorld() const
{
	return GetOuter()->GetWorld();
}
