// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Multithreading/LoadSlotsTask.h"

#include "Misc/SlotHelpers.h"
#include "SaveFileHelpers.h"
#include "SaveManager.h"

#include <HAL/PlatformFilemanager.h>


void FLoadSlotsTask::DoWork()
{
	if (!Manager)
	{
		return;
	}

	TArray<FString> FileNames;
	const bool bLoadingSingleInfo = !SlotName.IsNone();
	if (bLoadingSingleInfo)
	{
		FileNames.Add(SlotName.ToString());
	}
	else
	{
		FSlotHelpers::FindSlotFileNames(FileNames);
	}

	TArray<FSaveFile> LoadedFiles;
	LoadedFiles.Reserve(FileNames.Num());
	for (const FString& FileName : FileNames)
	{
		// Load all files
		FScopedFileReader Reader(FSaveFileHelpers::GetSlotPath(FileName));
		if (Reader.IsValid())
		{
			auto& File = LoadedFiles.AddDefaulted_GetRef();
			File.Read(Reader, true);
		}
	}

	// For cache friendlyness, we deserialize infos after loading all the files
	LoadedSlots.Reserve(LoadedFiles.Num());
	for (const auto& File : LoadedFiles)
	{
		LoadedSlots.Add(Cast<USaveSlot>(
			FSaveFileHelpers::DeserializeObject(nullptr, File.InfoClassName, Manager, File.InfoBytes)
		));
	}

	if (!bLoadingSingleInfo && bSortByRecent)
	{
		LoadedSlots.Sort([](const USaveSlot& A, const USaveSlot& B) {
			return A.Stats.SaveDate > B.Stats.SaveDate;
		});
	}
}

void FLoadSlotsTask::AfterFinish()
{
	for (auto& Slot : LoadedSlots)
	{
		Slot->ClearInternalFlags(EInternalObjectFlags::Async);
	}
	Delegate.ExecuteIfBound(LoadedSlots);
}
