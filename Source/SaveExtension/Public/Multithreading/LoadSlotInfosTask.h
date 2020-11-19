// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <Async/AsyncWork.h>
#include "FileAdapter.h"
#include "Multithreading/Delegates.h"

#include "SlotInfo.h"

class USaveManager;


/**
 * FLoadSlotInfosTask
 * Async task to load one or many slot infos
 */
class FLoadSlotInfosTask : public FNonAbandonableTask
{
protected:

	const USaveManager* Manager;

	const bool bSortByRecent = false;
	// If not empty, only this specific slot will be loaded
	const FName SlotName;

	TArray<USlotInfo*> LoadedSlots;

	FOnSlotInfosLoaded Delegate;


public:

	/** All infos Constructor */
	explicit FLoadSlotInfosTask(const USaveManager* Manager, bool bInSortByRecent, const FOnSlotInfosLoaded& Delegate)
		: Manager(Manager)
		, bSortByRecent(bInSortByRecent)
		, Delegate(Delegate)
	{}

	/** One info Constructor */
	explicit FLoadSlotInfosTask(USaveManager* Manager, FName SlotName)
		: Manager(Manager)
		, SlotName(SlotName)
	{}

	void DoWork();

	/** Called after the task has finished */
	void AfterFinish();

	const TArray<USlotInfo*>& GetLoadedSlots() const
	{
		return LoadedSlots;
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLoadAllSlotInfosTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};
