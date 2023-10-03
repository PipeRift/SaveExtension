// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "ISaveExtension.h"
#include "LevelFilter.h"
#include "SaveSlotData.h"

#include <Engine/Level.h>
#include <Engine/LevelStreaming.h>
#include <GameFramework/Actor.h>


class USaveManager;

enum class ESETaskType : uint8
{
	None,
	Load,
	Save
};

/**
 * Base class for managing the data of SaveSlot file
 */
struct FSEDataTask
{
	ESETaskType Type = ESETaskType::None;
private:
	bool bRunning = false;
	bool bFinished = false;
	bool bSucceeded = false;

protected:

	TObjectPtr<USaveManager> Manager;
	TObjectPtr<USaveSlotData> SlotData;
	float MaxFrameMs = 0.f;


public:
	FSEDataTask(USaveManager* Manager, USaveSlot* Slot, ESETaskType Type)
		: Type(Type)
		, Manager(Manager)
		, SlotData(Slot->GetData())
		, MaxFrameMs(Slot->GetMaxFrameMs())
	{}
	virtual ~FSEDataTask() = default;

	FSEDataTask& Start();

	virtual void Tick(float DeltaTime) {}

	void Finish(bool bSuccess);

	bool IsRunning() const
	{
		return bRunning;
	}
	bool IsFinished() const
	{
		return bFinished;
	}
	bool IsSucceeded() const
	{
		return IsFinished() && bSucceeded;
	}
	bool IsFailed() const
	{
		return IsFinished() && !bSucceeded;
	}
	bool IsScheduled() const;

	virtual void OnTick(float DeltaTime) {}

protected:
	virtual void OnStart() {}

	virtual void OnFinish(bool bSuccess) {}

	void BakeAllFilters();

	FLevelRecord* FindLevelRecord(const ULevelStreaming* Level) const;

	float GetTimeMilliseconds() const
	{
		return FPlatformTime::ToMilliseconds(FPlatformTime::Cycles());
	}

	UWorld* GetWorld() const;
};


/////////////////////////////////////////////////////
// FSlotDataActorsTask
// Async task to serialize actors from a level.
class FSlotDataActorsTask : public FNonAbandonableTask
{
protected:
	const bool bIsSync;
	/** USE ONLY IF SYNC */
	const UWorld* const World;
	/** USE ONLY IF SYNC */
	USaveSlotData* SlotData;

	const FSELevelFilter& Filter;


	FSlotDataActorsTask(
		const bool bInIsSync, const UWorld* InWorld, USaveSlotData* InSlotData, const FSELevelFilter& Filter)
		: bIsSync(bInIsSync)
		, World(InWorld)
		, SlotData(InSlotData)
		, Filter(Filter)
	{}
};
