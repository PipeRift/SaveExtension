// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "SlotDataTask.h"

#include <Engine/Level.h>
#include <Engine/LevelStreaming.h>
#include <GameFramework/Actor.h>

#include "SavePreset.h"
#include "SlotData.h"

#include "MTTask_SerializeActors.h"
#include "Multithreading/SaveFileTask.h"
#include "SlotDataTask_Saver.generated.h"


/** Called when game has been saved
 * @param SlotInfo the saved slot. Null if save failed
 */
DECLARE_DELEGATE_OneParam(FOnGameSaved, USlotInfo*);


/**
* Manages the saving process of a SaveData file
*/
UCLASS()
class USlotDataTask_Saver : public USlotDataTask
{
	GENERATED_BODY()

	bool bOverride;
	bool bSaveThumbnail;
	int32 Slot;
	int32 Width;
	int32 Height;

	FOnGameSaved Delegate;

protected:

	// Async variables
	TWeakObjectPtr<ULevel> CurrentLevel;
	TWeakObjectPtr<ULevelStreaming> CurrentSLevel;

	int32 CurrentActorIndex;
	TArray<TWeakObjectPtr<AActor>> CurrentLevelActors;

	/** Begin AsyncTasks */
	TArray<FAsyncTask<FMTTask_SerializeActors>> Tasks;
	FAsyncTask<FSaveFileTask>* SaveInfoTask;
	FAsyncTask<FSaveFileTask>* SaveDataTask;
	/** End AsyncTasks */


public:

	USlotDataTask_Saver()
		: USlotDataTask()
		, SaveInfoTask(nullptr)
		, SaveDataTask(nullptr)
	{}

	auto Setup(int32 InSlot, bool bInOverride, bool bInSaveThumbnail, const int32 InWidth, const int32 InHeight)
	{
		Slot = InSlot;
		bOverride = bInOverride;
		bSaveThumbnail = bInSaveThumbnail;
		Width = InWidth;
		Height = InHeight;
		return this;
	}

	auto Bind(const FOnGameSaved& OnSaved) { Delegate = OnSaved; return this; }

	virtual void OnStart() override;
	virtual void Tick(float DeltaTime) override
	{
		// If save file tasks exist and are both done
		if (SaveInfoTask && SaveDataTask && SaveInfoTask->IsDone() && SaveDataTask->IsDone())
			Finish(true);
	}
	virtual void OnFinish(bool bSuccess) override;
	virtual void BeginDestroy() override;

protected:

	/** BEGIN Serialization */
	void SerializeSync();
	void SerializeLevelSync(const ULevel* Level, int32 AssignedThreads, const ULevelStreaming* StreamingLevel = nullptr);

	void SerializeASync();
	void SerializeLevelASync(const ULevel* Level, const ULevelStreaming* StreamingLevel = nullptr);

	/** Serializes all world actors. */
	void SerializeWorld();
	/** END Serialization */

	void RunScheduledTasks();

private:

	/** BEGIN FileSaving */
	void SaveFile(const FString& InfoName, const FString& DataName);
	/** End FileSaving */
};
