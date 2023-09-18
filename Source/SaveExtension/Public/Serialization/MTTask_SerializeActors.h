// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "MTTask.h"
#include "MTTask_SerializeActors.h"
#include "Serialization/LevelRecords.h"
#include "Serialization/Records.h"

#include <Async/AsyncWork.h>
#include <Engine/LevelScriptActor.h>
#include <GameFramework/Actor.h>
#include <GameFramework/Controller.h>


class USaveSlotData;


/** Called when game has been saved
 * @param Slot the saved slot. Null if save failed
 */
DECLARE_DELEGATE_OneParam(FOnGameSaved, USaveSlot*);


/////////////////////////////////////////////////////
// FMTTask_SerializeActors
// Async task to serialize actors from a level.
class FMTTask_SerializeActors : public FMTTask
{
	const TArray<AActor*>* const LevelActors;
	const int32 StartIndex;
	const int32 Num;
	const bool bStoreGameInstance = false;

	/** USE ONLY FOR DUMPING DATA */
	FLevelRecord* LevelRecord = nullptr;

	FActorRecord LevelScriptRecord;
	TArray<FActorRecord> ActorRecords;


public:
	FMTTask_SerializeActors(const UWorld* World, USaveSlotData* SlotData,
		const TArray<AActor*>* const InLevelActors, const int32 InStartIndex, const int32 InNum,
		bool bStoreGameInstance, FLevelRecord* InLevelRecord, const FSELevelFilter& Filter)
		: FMTTask(false, World, SlotData, Filter)
		, LevelActors(InLevelActors)
		, StartIndex(InStartIndex)
		, Num(InNum)
		, bStoreGameInstance(bStoreGameInstance)
		, LevelRecord(InLevelRecord)
		, LevelScriptRecord{}
		, ActorRecords{}
	{
		// No apparent performance benefit
		// ActorRecords.Reserve(Num);
	}

	void DoWork();

	/** Called after task has completed to recover resulting information */
	void DumpData()
	{
		if (LevelScriptRecord.IsValid())
			LevelRecord->LevelScript = LevelScriptRecord;

		// Shrink not needed. Move wont keep reserved space
		LevelRecord->Actors.Append(MoveTemp(ActorRecords));
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FMTTask_SerializeActors, STATGROUP_ThreadPoolAsyncTasks);
	}

private:
	void SerializeGameInstance();

	/** Serializes an actor into this Actor Record */
	bool SerializeActor(const AActor* Actor, FActorRecord& Record) const;

	/** Serializes the components of an actor into a provided Actor Record */
	inline void SerializeActorComponents(
		const AActor* Actor, FActorRecord& ActorRecord, int8 indent = 0) const;
};
