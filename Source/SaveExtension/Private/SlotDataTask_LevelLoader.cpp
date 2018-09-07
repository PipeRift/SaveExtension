// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "SlotDataTask_LevelLoader.h"


/////////////////////////////////////////////////////
// USaveDataTask_LevelLoader

void USlotDataTask_LevelLoader::OnStart()
{
	if (SlotData && StreamingLevel && StreamingLevel->IsLevelLoaded())
	{
		if (Preset->GetAsyncMode() == ESaveASyncMode::LoadAsync ||
			Preset->GetAsyncMode() == ESaveASyncMode::SaveAndLoadAsync)
		{
			DeserializeLevelASync(StreamingLevel->GetLoadedLevel(), StreamingLevel);
		}
		else
		{
			DeserializeLevelSync(StreamingLevel->GetLoadedLevel(), StreamingLevel);
			FinishedDeserializing();
		}
		return;
	}
	Finish();
}

void USlotDataTask_LevelLoader::DeserializeASyncLoop(float StartMS /*= 0.0f*/)
{
	FLevelRecord * LevelRecord = FindLevelRecord(CurrentSLevel.Get());
	if (!LevelRecord)
	{
		Finish();
		return;
	}

	if (StartMS <= 0)
		StartMS = GetTimeMilliseconds();

	// Continue Iterating actors every tick
	for (; CurrentActorIndex < CurrentLevelActors.Num(); ++CurrentActorIndex)
	{
		AActor* Actor{ CurrentLevelActors[CurrentActorIndex].Get() };
		if (Actor)
		{
			DeserializeLevel_Actor(Actor, *LevelRecord);

			const float CurrentMS = GetTimeMilliseconds();
			// If x milliseconds passed, continue on next frame
			if (CurrentMS - StartMS >= MaxFrameMs)
				return;
		}
	}

	// All levels deserialized
	FinishedDeserializing();
}
