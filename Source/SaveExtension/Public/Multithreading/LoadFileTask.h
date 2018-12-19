// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include <Async/AsyncWork.h>
#include "FileAdapter.h"


/////////////////////////////////////////////////////
// FLoadFileTask
// Async task to load a File
class FLoadFileTask : public FNonAbandonableTask {
protected:

	USaveGame* SaveGame;
	const FString SlotName;

public:

	explicit FLoadFileTask(const FString& SlotName) :
		SaveGame(nullptr),
		SlotName(SlotName)
	{}

	void DoWork() {
		SaveGame = FFileAdapter::LoadFile(SlotName);
	}

	USaveGame* GetSaveGame() { return SaveGame; }

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLoadFileTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};
