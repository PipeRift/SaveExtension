// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "LoadSlotInfoTask.h"

#include <HAL/PlatformFilemanager.h>

#include "FileAdapter.h"
#include "SavePreset.h"
#include "SaveManager.h"


void FLoadSlotInfoTask::DoWork()
{
	if (!Manager)
		return;

	if (SlotId >= 0)
	{
		const FString Card = Manager->GenerateSaveSlotName(SlotId);
		LoadedSlot = Cast<USlotInfo>(FFileAdapter::LoadFile(Card));
	}
}
