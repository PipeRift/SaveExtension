// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "SlotDataTask_LevelSaver.h"


/////////////////////////////////////////////////////
// FSaveDataTask_LevelSaver

void USlotDataTask_LevelSaver::OnStart()
{
	if (SlotData && StreamingLevel && StreamingLevel->IsLevelLoaded())
	{
		SerializeLevelSync(StreamingLevel->GetLoadedLevel(), 1, StreamingLevel);
		//TODO: With async tasks Serializelevel will take charge of finishing
		//return;
	}
	Finish(true);
}
