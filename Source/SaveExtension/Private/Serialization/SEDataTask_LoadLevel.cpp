// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Serialization/SEDataTask_LoadLevel.h"


/////////////////////////////////////////////////////
// USaveDataTask_LevelLoader

void FSEDataTask_LoadLevel::OnStart()
{
	if (!SlotData || !StreamingLevel || !StreamingLevel->IsLevelLoaded())
	{
		Finish(false);
		return;
	}

	FLevelRecord* LevelRecord = FindLevelRecord(StreamingLevel);
	if (!LevelRecord)
	{
		Finish(false);
		return;
	}

	PrepareLevel(StreamingLevel->GetLoadedLevel(), *LevelRecord);

	if (Slot->IsFrameSplitLoad())
	{
		DeserializeLevelASync(StreamingLevel->GetLoadedLevel(), StreamingLevel);
	}
	else
	{
		DeserializeLevelSync(StreamingLevel->GetLoadedLevel(), StreamingLevel);
		FinishedDeserializing();
		return;
	}
}

void FSEDataTask_LoadLevel::DeserializeASyncLoop(float StartMS /*= 0.0f*/)
{

	if (StartMS <= 0)
	{
		StartMS = GetTimeMilliseconds();
	}

	FLevelRecord& LevelRecord = *FindLevelRecord(CurrentSLevel.Get());

	// Continue Iterating actors every tick
	for (; CurrentActorIndex < LevelRecord.RecordsToActors.Num(); ++CurrentActorIndex)
	{
		auto& RecordToActor = LevelRecord.RecordsToActors[CurrentActorIndex];

		const FActorRecord* Record = RecordToActor.Key;
		AActor* Actor = RecordToActor.Value.Get();
		check(Record);
		if (!Actor)
		{
			continue;
		}
		DeserializeActor(Actor, *Record, LevelRecord);

		const float CurrentMS = GetTimeMilliseconds();
		if (CurrentMS - StartMS >= MaxFrameMs)
		{
			// If x milliseconds passed, stop and continue on next frame
			return;
		}
	}

	// All levels deserialized
	FinishedDeserializing();
}
