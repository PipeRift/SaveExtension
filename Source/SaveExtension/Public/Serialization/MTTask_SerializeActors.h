// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "MTTask_SerializeActors.h"

#include <GameFramework/Actor.h>
#include <Engine/LevelScriptActor.h>
#include <GameFramework/Controller.h>
#include <AIController.h>
#include <Async/AsyncWork.h>

#include "SavePreset.h"

#include "MTTask.h"


class USlotData;

/** Called when game has been saved
 * @param SlotInfo the saved slot. Null if save failed
 */
DECLARE_DELEGATE_OneParam(FOnGameSaved, USlotInfo*);


/////////////////////////////////////////////////////
// FMTTask_SerializeActors
// Async task to serialize actors from a level.
class FMTTask_SerializeActors : public FMTTask
{
	const TArray<AActor*>* const LevelActors;
	const int32 StartIndex;
	const int32 Num;

	bool bStoreMainActors;

	/** USE ONLY FOR DUMPING DATA */
	FLevelRecord* LevelRecord;

	FActorRecord LevelScriptRecord;
	TArray<FActorRecord> ActorRecords;
	TArray<FControllerRecord> AIControllerRecords;


public:

	explicit FMTTask_SerializeActors(const bool bStoreMainActors, const UWorld* World, USlotData* SlotData, const TArray<AActor*>* const InLevelActors, const int32 InStartIndex, const int32 InNum, FLevelRecord* InLevelRecord, const USavePreset& Preset) :
		FMTTask(false, World, SlotData, Preset),
		LevelActors(InLevelActors),
		StartIndex(InStartIndex),
		Num(InNum),
		LevelRecord(InLevelRecord),
		LevelScriptRecord{}, ActorRecords{}, AIControllerRecords{}
	{
		// No apparent performance benefit
		// ActorRecords.Reserve(Num);
	}

	void DoWork();

	/** Called after task has completed to recover resulting information */
	void DumpData() {
		if (LevelScriptRecord.IsValid())
			LevelRecord->LevelScript = LevelScriptRecord;

		// Shrink not needed. Move wont keep reserved space
		LevelRecord->Actors.Append(MoveTemp(ActorRecords));
		LevelRecord->AIControllers.Append(MoveTemp(AIControllerRecords));
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FMTTask_SerializeActors, STATGROUP_ThreadPoolAsyncTasks);
	}

private:

	void SerializeGameInstance();

	/** Serializes an actor into this Controller Record */
	bool SerializeController(const AController* Actor, FControllerRecord& Record) const;

	/** Serializes an actor into this Actor Record */
	bool SerializeActor(const AActor* Actor, FActorRecord& Record) const;

	/** Serializes the components of an actor into a provided Actor Record */
	inline void SerializeActorComponents(const AActor* Actor, FActorRecord& ActorRecord, int8 indent = 0) const;
};
