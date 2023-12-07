// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "LevelFilter.h"
#include "SaveSlotData.h"

#include <Engine/Level.h>
#include <Engine/LevelStreaming.h>
#include <GameFramework/Actor.h>


class USaveManager;
class USaveSlotData;


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


public:
	FSEDataTask(USaveManager* Manager, ESETaskType Type) : Type(Type), Manager(Manager) {}
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

	FLevelRecord* FindLevelRecord(USaveSlotData& Data, const ULevelStreaming* Level) const;

	float GetTimeMilliseconds() const
	{
		return FPlatformTime::ToMilliseconds(FPlatformTime::Cycles());
	}

	UWorld* GetWorld() const;


public:
	static FString GetWorldName(const UWorld* World);
};
