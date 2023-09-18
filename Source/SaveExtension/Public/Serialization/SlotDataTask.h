// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "ISaveExtension.h"
#include "LevelFilter.h"
#include "SaveSlotData.h"

#include <Engine/Level.h>
#include <Engine/LevelStreaming.h>
#include <GameFramework/Actor.h>

#include "SlotDataTask.generated.h"


class USaveManager;


/**
 * Base class for managing a SaveData file
 */
UCLASS()
class USaveSlotDataTask : public UObject
{
	GENERATED_BODY()

private:
	uint8 bRunning : 1;
	uint8 bFinished : 1;
	uint8 bSucceeded : 1;

protected:
	UPROPERTY()
	USaveSlotData* SlotData;

	UPROPERTY()
	float MaxFrameMs = 0.f;

public:
	USaveSlotDataTask() : Super(), bRunning(false), bFinished(false) {}

	void Prepare(USaveSlot* Slot)
	{
		SlotData = Slot->GetData();
		MaxFrameMs = Slot->GetMaxFrameMs();
	}

	USaveSlotDataTask* Start();

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

	USaveManager* GetManager() const;

	void BakeAllFilters();

	const FSELevelFilter& GetGlobalFilter() const;
	const FSELevelFilter& GetLevelFilter(const FLevelRecord& Level) const;

	FLevelRecord* FindLevelRecord(const ULevelStreaming* Level) const;

	//~ Begin UObject Interface
	virtual UWorld* GetWorld() const override;
	//~ End UObject Interface

	FORCEINLINE float GetTimeMilliseconds() const
	{
		return FPlatformTime::ToMilliseconds(FPlatformTime::Cycles());
	}
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
