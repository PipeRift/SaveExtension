// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "Multithreading/DeleteSlotsTask.h"

#include <HAL/PlatformFilemanager.h>

#include "FileAdapter.h"
#include "SavePreset.h"
#include "SaveManager.h"
#include "Misc/SlotHelpers.h"
#include "HAL/FileManager.h"


FDeleteSlotsTask::FDeleteSlotsTask(const USaveManager* InManager, int32 SlotId)
	: Manager(InManager)
	, bSuccess(false)
{
	check(Manager);
	SpecificSlotName = Manager->GenerateSlotName(SlotId);
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
