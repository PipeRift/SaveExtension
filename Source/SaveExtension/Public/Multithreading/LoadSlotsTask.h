// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "Multithreading/Delegates.h"
#include "SaveFileHelpers.h"
#include "SaveSlot.h"

#include <Async/AsyncWork.h>


class USaveManager;


/**
 * FLoadSlotsTask
 * Async task to load one or many slot infos
 */
class FLoadSlotsTask : public FNonAbandonableTask
{
protected:
	const USaveManager* Manager;

	const bool bSortByRecent = false;
	// If not empty, only this specific slot will be loaded
	const FName SlotName;

	TArray<USaveSlot*> LoadedSlots;

	FOnSlotsLoaded Delegate;


public:
	/** All infos Constructor */
	explicit FLoadSlotsTask(const USaveManager* Manager, bool bInSortByRecent, const FOnSlotsLoaded& Delegate)
		: Manager(Manager)
		, bSortByRecent(bInSortByRecent)
		, Delegate(Delegate)
	{}

	/** One info Constructor */
	explicit FLoadSlotsTask(USaveManager* Manager, FName SlotName) : Manager(Manager), SlotName(SlotName) {}

	void DoWork();

	/** Called after the task has finished */
	void AfterFinish();

	const TArray<USaveSlot*>& GetLoadedSlots() const
	{
		return LoadedSlots;
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLoadAllSlotsTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};
