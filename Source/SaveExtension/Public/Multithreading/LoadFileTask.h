// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "SaveFileHelpers.h"
#include "SaveManager.h"
#include <Async/AsyncWork.h>


/////////////////////////////////////////////////////
// FLoadFileTask
// Async task to load a File
class FLoadFileTask : public FNonAbandonableTask
{
protected:
	TWeakObjectPtr<USaveManager> Manager;
	const FString SlotName;

	TWeakObjectPtr<USaveSlot> LastSlot;
	TWeakObjectPtr<USaveSlotData> LastSlotData;

	TWeakObjectPtr<USaveSlot> Slot;


public:
	explicit FLoadFileTask(USaveManager* Manager, USaveSlot* LastSlot, FStringView SlotName);
	~FLoadFileTask();

	void DoWork();

	/** Game thread */
	USaveSlot* GetInfo() const
	{
		return Slot.Get();
	}

	USaveSlotData* GetData() const
	{
		return Slot.IsValid()? Slot->GetData() : nullptr;
	}

	TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLoadFileTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};
