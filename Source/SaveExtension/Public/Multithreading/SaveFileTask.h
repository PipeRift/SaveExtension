// Copyright 2015-2018 Piperift. All Rights Reserved.

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

	explicit FSaveFileTask(USaveGame* SaveGame, const FString& SlotName, const bool bUseCompression) :
		SaveGame(SaveGame),
		SlotName(SlotName),
		bUseCompression(bUseCompression)
	{}

	void DoWork() {
		FFileAdapter::SaveFile(SaveGame, SlotName, bUseCompression);
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FSaveFileTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};
