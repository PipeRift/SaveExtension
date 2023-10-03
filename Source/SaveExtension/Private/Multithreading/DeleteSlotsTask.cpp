// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Multithreading/DeleteSlotsTask.h"

#include "HAL/FileManager.h"
#include "Misc/SlotHelpers.h"
#include "SaveFileHelpers.h"
#include "SaveManager.h"

#include <HAL/PlatformFilemanager.h>


FDeleteSlotsTask::FDeleteSlotsTask(const USaveManager* InManager, FName SlotName) : Manager(InManager)
{
	check(Manager);
	if (!SlotName.IsNone())
	{
		SpecificSlotName = SlotName.ToString();
	}
}

void FDeleteSlotsTask::DoWork()
{
	if (!SpecificSlotName.IsEmpty())
	{
		// Delete a single slot by id
		const FString ScreenshotPath = FSaveFileHelpers::GetThumbnailPath(SpecificSlotName);
		bool bIsDeleteSlotSuccess = FSaveFileHelpers::DeleteFile(SpecificSlotName);
		bool bIsDeleteScreenshotSuccess = IFileManager::Get().Delete(*ScreenshotPath, true);
		bSuccess = bIsDeleteSlotSuccess || bIsDeleteScreenshotSuccess;
	}
	else
	{
		TArray<FString> FoundSlots;
		FSlotHelpers::FindSlotFileNames(FoundSlots);

		for (const FString& File : FoundSlots)
		{
			FSaveFileHelpers::DeleteFile(File);
		}
		bSuccess = true;
	}
}
