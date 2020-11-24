// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "Serialization/SlotDataTask_LevelSaver.h"


/////////////////////////////////////////////////////
// FSaveDataTask_LevelSaver

void USlotDataTask_LevelSaver::OnStart()
{
	if (SlotData && StreamingLevel && StreamingLevel->IsLevelLoaded())
	{
		FLevelRecord* LevelRecord = FindLevelRecord(StreamingLevel);
		if (!LevelRecord)
		{
			Finish(false);
			return;
		}

		GetLevelFilter(*LevelRecord).BakeAllowedClasses();

		const int32 NumberOfThreads = FMath::Max(1, FPlatformMisc::NumberOfWorkerThreadsToSpawn());
		SerializeLevelSync(StreamingLevel->GetLoadedLevel(), NumberOfThreads, StreamingLevel);

		RunScheduledTasks();

		Finish(true);
		return;
	}
	Finish(false);
}
