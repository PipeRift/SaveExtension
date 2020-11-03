// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "Multithreading/LoadAllSlotInfosTask.h"

#include <HAL/PlatformFilemanager.h>

#include "FileAdapter.h"
#include "SavePreset.h"
#include "SaveManager.h"
#include "Misc/SlotHelpers.h"


void FLoadAllSlotInfosTask::DoWork()
{
	if (!Manager)
		return;

	TArray<FString> FileNames;
	FSlotHelpers::FindSlotFileNames(FileNames);

	LoadedSlots.Reserve(FileNames.Num());

	for (const FString& File : FileNames)
	{
		if (USlotInfo* Info = LoadInfoFromFile(File))
		{
			LoadedSlots.Add(Info);
		}
	}

	if (bSortByRecent)
	{
		LoadedSlots.Sort([](const USlotInfo& A, const USlotInfo& B) {
			return A.SaveDate > B.SaveDate;
		});
	}
}

USlotInfo* FLoadAllSlotInfosTask::LoadInfoFromFile(const FString Name) const
{
	USlotInfo* LoadedInfo = nullptr;
	USlotData* LoadedData = nullptr;
	FFileAdapter::LoadFile(Name, LoadedInfo, LoadedData, false);
	return LoadedInfo;
}
