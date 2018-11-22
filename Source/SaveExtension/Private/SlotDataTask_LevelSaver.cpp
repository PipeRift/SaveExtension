// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "SlotDataTask_LevelSaver.h"


/////////////////////////////////////////////////////
// FSaveDataTask_LevelSaver

void USlotDataTask_LevelSaver::OnStart()
{
	if (SlotData && StreamingLevel && StreamingLevel->IsLevelLoaded())
	{
		const int32 NumberOfThreads = FMath::Max(1, FPlatformMisc::NumberOfWorkerThreadsToSpawn());
		SerializeLevelSync(StreamingLevel->GetLoadedLevel(), NumberOfThreads, StreamingLevel);
		Finish(true);
		return;
	}
	Finish(false);
}
