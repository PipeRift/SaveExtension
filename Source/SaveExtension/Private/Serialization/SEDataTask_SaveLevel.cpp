// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Serialization/SEDataTask_SaveLevel.h"


/////////////////////////////////////////////////////
// FSaveDataTask_LevelSaver

void FSEDataTask_SaveLevel::OnStart()
{
	if (SlotData && StreamingLevel && StreamingLevel->IsLevelLoaded())
	{
		FLevelRecord* LevelRecord = FindLevelRecord(StreamingLevel);
		if (!LevelRecord)
		{
			Finish(false);
			return;
		}

		const int32 NumberOfThreads = FMath::Max(1, FPlatformMisc::NumberOfWorkerThreadsToSpawn());
		SerializeLevelSync(StreamingLevel->GetLoadedLevel(), NumberOfThreads, StreamingLevel);

		RunScheduledTasks();

		Finish(true);
		return;
	}
	Finish(false);
}
