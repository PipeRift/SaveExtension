// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "Multithreading/LoadSlotInfosTask.h"

#include <HAL/PlatformFilemanager.h>

#include "FileAdapter.h"
#include "SavePreset.h"
#include "SaveManager.h"
#include "Misc/SlotHelpers.h"


void FLoadSlotInfosTask::DoWork()
{
	if (!Manager)
	{
		return;
	}

	TArray<FString> FileNames;
	const bool bLoadingSingleInfo = !SlotName.IsNone();
	if(bLoadingSingleInfo)
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
		FScopedFileReader Reader(FFileAdapter::GetSlotPath(FileName));
		if(Reader.IsValid())
		{
			auto& File = LoadedFiles.AddDefaulted_GetRef();
			File.Read(Reader, true);
		}
	}

	// For cache friendlyness, we deserialize infos after loading all the files
	LoadedSlots.Reserve(LoadedFiles.Num());
	for (const auto& File : LoadedFiles)
	{
		LoadedSlots.Add(File.CreateAndDeserializeInfo(Manager));
	}

	if (!bLoadingSingleInfo && bSortByRecent)
	{
		LoadedSlots.Sort([](const USlotInfo& A, const USlotInfo& B) {
			return A.SaveDate > B.SaveDate;
		});
	}
}

void FLoadSlotInfosTask::AfterFinish()
{
	for(auto& Slot : LoadedSlots)
	{
		Slot->ClearInternalFlags(EInternalObjectFlags::Async);
	}
	Delegate.ExecuteIfBound(LoadedSlots);
}
