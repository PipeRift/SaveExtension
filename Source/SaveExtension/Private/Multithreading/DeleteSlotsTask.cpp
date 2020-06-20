// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "Multithreading/DeleteSlotsTask.h"

#include <HAL/PlatformFilemanager.h>

#include "FileAdapter.h"
#include "SavePreset.h"
#include "SaveManager.h"
#include "Misc/SlotHelpers.h"
#include "HAL/FileManager.h"


void FDeleteSlotsTask::DoWork()
{
	if (SpecificSlotId > 0)
	{
		// Delete a single slot by id
		const FString InfoSlot = Manager->GenerateSlotInfoName(SpecificSlotId);
		const FString DataSlot = Manager->GenerateSlotDataName(SpecificSlotId);
		FString ScreenshotPath = FString::Printf(TEXT("%sSaveGames/%i_%s.%s"), *FPaths::ProjectSavedDir(), SpecificSlotId, *FString("SaveScreenshot"), TEXT("png"));
		bool bIsDeleteInfoSlotSuccess = FFileAdapter::DeleteFile(InfoSlot);
		bool bIsDeleteDataSlotSuccess = FFileAdapter::DeleteFile(DataSlot);
		bool bIsDeleteScreenshotSuccess = IFileManager::Get().Delete(*ScreenshotPath, true);
		bSuccess = bIsDeleteInfoSlotSuccess || bIsDeleteDataSlotSuccess || bIsDeleteScreenshotSuccess;
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
