// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <Async/AsyncWork.h>
#include "FileAdapter.h"


/////////////////////////////////////////////////////
// FLoadFileTask
// Async task to load a File
class FLoadFileTask : public FNonAbandonableTask {
protected:

	const FString SlotName;

	bool bSucceededLoad;
	FSaveFileHeader FileHeader;
	TArray<uint8> DataBytes;

public:

	explicit FLoadFileTask(const FString& InSlotName)
		: SlotName(InSlotName)
		, bSucceededLoad{false}
		, FileHeader{}
		, DataBytes{}
	{}

	void DoWork() {
		// This task splits FFileAdapter::LoadFile for multithreading compatibility
		bSucceededLoad = FFileAdapter::LoadFileBytes(SlotName, FileHeader, DataBytes);
	}

	// Create and return the resulting loaded SaveGame object. Call when task from Gamethread when task is finished
	USaveGame* GetSaveGame() {
		if (bSucceededLoad)
		{
			return FFileAdapter::CreateFromBytes(FileHeader, DataBytes);
		}
		return nullptr;
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLoadFileTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};
