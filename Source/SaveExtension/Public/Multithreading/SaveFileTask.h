// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "SaveFileHelpers.h"

#include <Async/AsyncWork.h>


/////////////////////////////////////////////////////
// FSaveFileTask
// Async task to save a File
class FSaveFileTask : public FNonAbandonableTask
{
protected:
	USaveSlot* Info;
	const FString SlotName;
	const bool bUseCompression;

public:
	FSaveFileTask(USaveSlot* Info, const FString& InSlotName, const bool bInUseCompression)
		: Info(Info)
		, SlotName(InSlotName)
		, bUseCompression(bInUseCompression)
	{}

	void DoWork()
	{
		FSaveFileHelpers::SaveFile(SlotName, Info, bUseCompression);
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FSaveFileTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};
