// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "FileAdapter.h"
#include "Multithreading/Delegates.h"
#include "SaveSlot.h"

#include <Async/AsyncWork.h>


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

	TArray<USaveSlot*> LoadedSlots;

	FOnSlotInfosLoaded Delegate;


public:
	/** All infos Constructor */
	explicit FLoadSlotInfosTask(
		const USaveManager* Manager, bool bInSortByRecent, const FOnSlotInfosLoaded& Delegate)
		: Manager(Manager)
		, bSortByRecent(bInSortByRecent)
		, Delegate(Delegate)
	{}

	/** One info Constructor */
	explicit FLoadSlotInfosTask(USaveManager* Manager, FName SlotName) : Manager(Manager), SlotName(SlotName)
	{}

	void DoWork();

	/** Called after the task has finished */
	void AfterFinish();

	const TArray<USaveSlot*>& GetLoadedSlots() const
	{
		return LoadedSlots;
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLoadAllSlotInfosTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};
