// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "FileAdapter.h"

#include <Async/AsyncWork.h>


/////////////////////////////////////////////////////
// FLoadFileTask
// Async task to load a File
class FLoadFileTask : public FNonAbandonableTask
{
protected:
	TWeakObjectPtr<USaveManager> Manager;
	const FString SlotName;

	TWeakObjectPtr<USaveSlot> SlotInfo;
	TWeakObjectPtr<USaveSlotData> SlotData;


public:
	explicit FLoadFileTask(USaveManager* Manager, FStringView SlotName) : Manager(Manager), SlotName(SlotName)
	{}
	~FLoadFileTask()
	{
		if (SlotInfo.IsValid())
		{
			SlotInfo->ClearInternalFlags(EInternalObjectFlags::Async);
		}
		if (SlotData.IsValid())
		{
			SlotData->ClearInternalFlags(EInternalObjectFlags::Async);
		}
	}

	void DoWork()
	{
		FScopedFileReader FileReader(FFileAdapter::GetSlotPath(SlotName));
		if (FileReader.IsValid())
		{
			FSaveFile File;
			File.Read(FileReader, false);
			SlotInfo = File.CreateAndDeserializeInfo(Manager.Get());
			SlotData = File.CreateAndDeserializeData(Manager.Get());
		}
	}

	USaveSlot* GetInfo()
	{
		return SlotInfo.Get();
	}

	USaveSlotData* GetData()
	{
		return SlotData.Get();
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLoadFileTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};
