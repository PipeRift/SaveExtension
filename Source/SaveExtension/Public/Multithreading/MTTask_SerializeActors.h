// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "Multithreading/MTTask.h"
#include "Multithreading/MTTask_SerializeActors.h"
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
	const TArray<AActor*>* LevelActors;
	const int32 StartIndex = 0;
	const int32 Num = 0;
	const bool bStoreGameInstance = false;

	FLevelRecord* LevelRecord = nullptr;
	const FSELevelFilter* Filter = nullptr;


	FActorRecord LevelScriptRecord;
	TArray<FActorRecord> ActorRecords;


public:
	FMTTask_SerializeActors(UWorld* World, USaveSlotData* SlotData,
		const TArray<AActor*>* LevelActors, const int32 StartIndex, const int32 Num,
		bool bStoreGameInstance, FLevelRecord* LevelRecord, const FSELevelFilter* Filter)
		: FMTTask(false, World, SlotData)
		, LevelActors(LevelActors)
		, StartIndex(StartIndex)
		, Num(Num)
		, bStoreGameInstance(bStoreGameInstance)
		, LevelRecord(LevelRecord)
		, Filter(Filter)
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
