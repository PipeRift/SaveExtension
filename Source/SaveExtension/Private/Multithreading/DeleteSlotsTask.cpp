// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Multithreading/DeleteSlotsTask.h"

#include "FileAdapter.h"
#include "HAL/FileManager.h"
#include "Misc/SlotHelpers.h"
#include "SaveManager.h"
#include "SavePreset.h"

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
		const FString ScreenshotPath = FFileAdapter::GetThumbnailPath(SpecificSlotName);
		bool bIsDeleteSlotSuccess = FFileAdapter::DeleteFile(SpecificSlotName);
		bool bIsDeleteScreenshotSuccess = IFileManager::Get().Delete(*ScreenshotPath, true);
		bSuccess = bIsDeleteSlotSuccess || bIsDeleteScreenshotSuccess;
	}
	else
	{
		TArray<FString> FoundSlots;
		FSlotHelpers::FindSlotFileNames(FoundSlots);

		for (const FString& File : FoundSlots)
		{
			FFileAdapter::DeleteFile(File);
		}
		bSuccess = true;
	}
}
