// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <Async/AsyncWork.h>
#include "FileAdapter.h"


/////////////////////////////////////////////////////
// FLoadFileTask
// Async task to load a File
class FLoadFileTask : public FNonAbandonableTask {
protected:

	const FString SlotName;

	FSaveFile File;


public:

	explicit FLoadFileTask(const FString& InSlotName)
		: SlotName(InSlotName)
		, File{}
	{
	}

	void DoWork()
	{
		FScopedFileReader FileReader(FFileAdapter::GetSavePath(SlotName));
		if(FileReader.IsValid())
		{
			File.Read(FileReader, false);
		}
	}

	USlotInfo* GetInfo()
	{
		return File.CreateAndDeserializeInfo();
	}

	USlotData* GetData()
	{
		return File.CreateAndDeserializeData();
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLoadFileTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};
