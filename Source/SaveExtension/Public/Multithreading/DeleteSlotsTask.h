// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Async/AsyncWork.h>
#include "FileAdapter.h"

#include "SlotInfo.h"

class USaveManager;

// @param Amount of slots removed
DECLARE_DELEGATE(FOnSlotsDeleted);


/**
 * FLoadSlotInfosTask
 * Async task to load an SlotInfo
 */
class FDeleteSlotsTask : public FNonAbandonableTask {
protected:

	const USaveManager* const Manager;
	FString SpecificSlotName;

public:

	bool bSuccess;

	/** All infos Constructor */
	explicit FDeleteSlotsTask(const USaveManager* InManager, int32 SlotId = -1);

	void DoWork();

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FDeleteSlotsTask, STATGROUP_ThreadPoolAsyncTasks);
	}

private:

	USlotInfo* LoadInfoFromFile(const FString Name) const;
};
