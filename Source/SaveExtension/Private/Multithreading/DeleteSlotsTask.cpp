// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "Multithreading/DeleteSlotsTask.h"

#include <HAL/PlatformFilemanager.h>

#include "FileAdapter.h"
#include "SavePreset.h"
#include "SaveManager.h"
#include "Misc/SlotHelpers.h"


void FDeleteSlotsTask::DoWork()
{
	if (SpecificSlotId > 0)
	{
		// Delete a single slot by id
		const FString InfoSlot = Manager->GenerateSlotInfoName(SpecificSlotId);
		const FString DataSlot = Manager->GenerateSlotDataName(SpecificSlotId);
		bSuccess = FFileAdapter::DeleteFile(InfoSlot) ||
			       FFileAdapter::DeleteFile(DataSlot);
	}
	else
	{
		TArray<FString> FileNames;
		FSlotHelpers::GetSlotFileNames(FileNames);

		for (const FString& File : FileNames)
		{
			FFileAdapter::DeleteFile(File);
		}
		bSuccess = FileNames.Num() > 0;
	}
}
