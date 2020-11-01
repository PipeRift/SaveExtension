// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <Async/AsyncWork.h>
#include "FileAdapter.h"


/////////////////////////////////////////////////////
// FSaveFileTask
// Async task to save a File
class FSaveFileTask : public FNonAbandonableTask {
protected:

	USlotInfo* Info;
	USlotData* Data;
	const FString SlotName;
	const bool bUseCompression;

public:

	FSaveFileTask(USlotInfo* Info, USlotData* Data, const FString& InSlotName, const bool bInUseCompression) :
		Info(Info),
		Data(Data),
		SlotName(InSlotName),
		bUseCompression(bInUseCompression)
	{}

	void DoWork()
	{
		FFileAdapter::SaveFile(SlotName, Info, Data, bUseCompression);
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FSaveFileTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};
