// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "SaveFileHelpers.h"

#include <Async/AsyncWork.h>


/////////////////////////////////////////////////////
// FLoadFileTask
// Async task to load a File
class FLoadFileTask : public FNonAbandonableTask
{
protected:
	TWeakObjectPtr<USaveManager> Manager;
	const FString SlotName;

	TWeakObjectPtr<USaveSlot> Slot;
	TWeakObjectPtr<USaveSlotData> SlotData;


public:
	explicit FLoadFileTask(USaveManager* Manager, FStringView SlotName) : Manager(Manager), SlotName(SlotName)
	{}
	~FLoadFileTask()
	{
		if (Slot.IsValid())
		{
			Slot->ClearInternalFlags(EInternalObjectFlags::Async);
		}
		if (SlotData.IsValid())
		{
			SlotData->ClearInternalFlags(EInternalObjectFlags::Async);
		}
	}

	void DoWork()
	{
		FScopedFileReader FileReader(FSaveFileHelpers::GetSlotPath(SlotName));
		if (FileReader.IsValid())
		{
			FSaveFile File;
			File.Read(FileReader, false);
			USaveSlot* NewSlot = Slot.Get();
			File.CreateAndDeserializeSlot(NewSlot, Manager.Get());
			File.CreateAndDeserializeData(NewSlot);
			Slot = NewSlot;
		}
	}

	USaveSlot* GetInfo()
	{
		return Slot.Get();
	}

	USaveSlotData* GetData()
	{
		return SlotData.Get();
	}

	TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLoadFileTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};
