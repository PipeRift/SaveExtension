// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <Async/AsyncWork.h>
#include "FileAdapter.h"

#include "SlotInfo.h"


/**
 * FLoadSlotInfosTask
 * Async task to load an SlotInfo
 */
class FLoadSlotInfoTask : public FNonAbandonableTask {

public:

	DECLARE_DELEGATE_OneParam(FOnSlotInfoLoaded, USlotInfo*);

protected:

	const class USaveManager* const Manager;
	FString SlotName;

	USlotInfo* LoadedSlot;


public:

	explicit FLoadSlotInfoTask(const USaveManager* Manager, const int32 SlotId);

	void DoWork();

	/** Call only when task has finished */
	USlotInfo* GetLoadedSlot() {
		return LoadedSlot;
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLoadSlotInfoTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};
