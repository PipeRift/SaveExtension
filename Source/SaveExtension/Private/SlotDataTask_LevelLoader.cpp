// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "SlotDataTask_LevelLoader.h"


/////////////////////////////////////////////////////
// USaveDataTask_LevelLoader

void USlotDataTask_LevelLoader::OnStart()
{
	if (SlotData && StreamingLevel && StreamingLevel->IsLevelLoaded())
	{
		DeserializeLevelASync(StreamingLevel->GetLoadedLevel(), StreamingLevel);

		// Sync
		// DeserializeLevelSync(StreamingLevel->GetLoadedLevel(), StreamingLevel);
		// FinishedDeserializing();
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
		StartMS = FPlatformTime::ToMilliseconds(FPlatformTime::Cycles());

	// Continue Iterating actors every tick
	for (; CurrentActorIndex < CurrentLevelActors.Num(); ++CurrentActorIndex)
	{
		AActor* Actor{ CurrentLevelActors[CurrentActorIndex].Get() };
		if (Actor)
		{
			DeserializeLevel_Actor(Actor, *LevelRecord);

			const float CurrentMS = FPlatformTime::ToMilliseconds(FPlatformTime::Cycles());
			// If 5MS did pass
			if (CurrentMS - StartMS >= 5)
			{
				// Wait for next frame
				return;
			}
		}
	}

	// All levels deserialized
	FinishedDeserializing();
}
