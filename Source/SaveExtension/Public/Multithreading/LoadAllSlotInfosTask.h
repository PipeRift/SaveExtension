// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <Async/AsyncWork.h>
#include "FileAdapter.h"

#include "SlotInfo.h"

class USaveManager;

DECLARE_DELEGATE_OneParam(FOnAllInfosLoaded, const TArray<USlotInfo*>&);


/**
 * FLoadSlotInfosTask
 * Async task to load an SlotInfo
 */
class FLoadAllSlotInfosTask : public FNonAbandonableTask {
protected:

	const USaveManager* const Manager;
	const int32 SlotId;
	const bool bSortByRecent;

	FOnAllInfosLoaded Delegate;

	TArray<USlotInfo*> LoadedSlots;


public:

	/** All infos Constructor */
	explicit FLoadAllSlotInfosTask(const USaveManager* InManager, bool bInSortByRecent, const FOnAllInfosLoaded& InDelegate)
		: Manager(InManager)
		, SlotId(-1)
		, bSortByRecent(bInSortByRecent)
		, Delegate(InDelegate)
	{}

	void DoWork();

	/** Call only when task has finished */
	void CallDelegate() {
		Delegate.ExecuteIfBound(LoadedSlots);
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLoadAllSlotInfosTask, STATGROUP_ThreadPoolAsyncTasks);
	}

private:

	USlotInfo* LoadInfoFromFile(const FString Name) const;
};
