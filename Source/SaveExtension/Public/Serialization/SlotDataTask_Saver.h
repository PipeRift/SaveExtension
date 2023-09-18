// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "Delegates.h"
#include "ISaveExtension.h"
#include "MTTask_SerializeActors.h"
#include "Multithreading/SaveFileTask.h"
#include "SaveSlotData.h"
#include "SlotDataTask.h"

#include <AIController.h>
#include <Async/AsyncWork.h>
#include <Engine/Level.h>
#include <Engine/LevelScriptActor.h>
#include <Engine/LevelStreaming.h>
#include <GameFramework/Actor.h>
#include <GameFramework/Controller.h>

#include "SlotDataTask_Saver.generated.h"


/**
 * Manages the saving process of a SaveData file
 */
UCLASS()
class USaveSlotDataTask_Saver : public USaveSlotDataTask
{
	GENERATED_BODY()

	bool bOverride;
	bool bSaveThumbnail;
	FName SlotName;
	int32 Width;
	int32 Height;

	FOnGameSaved Delegate;

protected:
	UPROPERTY()
	USaveSlot* Slot;

	/** Start Async variables */
	TWeakObjectPtr<ULevel> CurrentLevel;
	TWeakObjectPtr<ULevelStreaming> CurrentSLevel;
	int32 CurrentActorIndex;
	TArray<TWeakObjectPtr<AActor>> CurrentLevelActors;
	/** End Async variables */

	/** Begin AsyncTasks */
	TArray<FAsyncTask<FMTTask_SerializeActors>> Tasks;
	FAsyncTask<FSaveFileTask>* SaveTask;
	/** End AsyncTasks */


public:
	USaveSlotDataTask_Saver() : USaveSlotDataTask(), SaveTask(nullptr) {}

	auto* Setup(
		FName InSlotName, bool bInOverride, bool bInSaveThumbnail, const int32 InWidth, const int32 InHeight)
	{
		SlotName = InSlotName;
		bOverride = bInOverride;
		bSaveThumbnail = bInSaveThumbnail;
		Width = InWidth;
		Height = InHeight;

		return this;
	}

	auto* Bind(const FOnGameSaved& OnSaved)
	{
		Delegate = OnSaved;
		return this;
	}

	// Where all magic happens
	virtual void OnStart() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnFinish(bool bSuccess) override;
	virtual void BeginDestroy() override;

protected:
	/** BEGIN Serialization */
	/** Serializes all world actors. */
	void SerializeWorld();

	void PrepareAllLevels(const TArray<ULevelStreaming*>& Levels);

	void SerializeLevelSync(
		const ULevel* Level, int32 AssignedThreads, const ULevelStreaming* StreamingLevel = nullptr);

	/** END Serialization */

	void RunScheduledTasks();

private:
	/** BEGIN FileSaving */
	void SaveFile();
	/** End FileSaving */
};
