// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "Multithreading/LoadSlotInfoTask.h"

#include <HAL/PlatformFilemanager.h>

#include "FileAdapter.h"
#include "SavePreset.h"
#include "SaveManager.h"


FLoadSlotInfoTask::FLoadSlotInfoTask(const USaveManager* Manager, const int32 SlotId)
	: Manager(Manager)
{
	check(Manager);
	SlotName = Manager->GenerateSlotName(SlotId);
}

void FLoadSlotInfoTask::DoWork()
{
	if (Manager && !SlotName.IsEmpty())
	{
		USlotData* LoadedData = nullptr;
		FFileAdapter::LoadFile(SlotName, LoadedSlot, LoadedData, false);
	}
}
