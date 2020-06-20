// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <Async/AsyncWork.h>
#include "FileAdapter.h"


/////////////////////////////////////////////////////
// FSaveFileTask
// Async task to save a File
class FSaveFileTask : public FNonAbandonableTask {
protected:

	USaveGame* SaveGame;
	const FString SlotName;
	const bool bUseCompression;

public:

	FSaveFileTask(USaveGame* InSaveGame, const FString& InSlotName, const bool bInUseCompression) :
		SaveGame(InSaveGame),
		SlotName(InSlotName),
		bUseCompression(bInUseCompression)
	{}

	void DoWork() {
		FFileAdapter::SaveFile(SaveGame, SlotName, bUseCompression);
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FSaveFileTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};
