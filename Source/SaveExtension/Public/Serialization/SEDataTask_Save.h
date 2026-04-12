// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "SEDataTask.h"

#include <AIController.h>
#include <Engine/Level.h>
#include <Engine/LevelScriptActor.h>
#include <Engine/LevelStreaming.h>
#include <GameFramework/Actor.h>
#include <GameFramework/Controller.h>
#include <Tasks/Task.h>


class USaveManager;
class USaveSlot;
class USaveSlotData;


/** Called when game has been saved
 * @param SaveSlot the saved slot. Null if save failed
 */
DECLARE_DELEGATE_OneParam(FOnGameSaved, USaveSlot*);


/**
 * Manages the saving process of a SaveData file
 */
struct FSEDataTask_Save : public FSEDataTask
{
	bool bOverride = false;
	bool bCaptureThumbnail = false;
	FName SlotName;
	int32 Width = 0;
	int32 Height = 0;

	FOnGameSaved Delegate;

protected:
	TObjectPtr<USaveSlot> Slot;
	TObjectPtr<USaveSlotData> SlotData;
	FSEClassFilter SubsystemFilter;


	UE::Tasks::TTask<bool> SaveFileTask;

	bool bWaitingThumbnail = false;

public:
	FSEDataTask_Save(USaveManager* Manager, USaveSlot* Slot);
	~FSEDataTask_Save();

	auto& Setup(
		FName InSlotName, bool bInOverride, bool bInSaveThumbnail, const int32 InWidth, const int32 InHeight)
	{
		SlotName = InSlotName;
		bOverride = bInOverride;
		bCaptureThumbnail = bInSaveThumbnail;
		Width = InWidth;
		Height = InHeight;

		return *this;
	}

	auto& Bind(const FOnGameSaved& OnSaved)
	{
		Delegate = OnSaved;
		return *this;
	}

	// Where all magic happens
	virtual void OnStart() override;
	virtual void Tick(float DelLoadtaTime) override;
	virtual void OnFinish(bool bSuccess) override;

protected:
	/** Serializes all world actors. */
	void SerializeWorld();
	void PrepareAllLevels(const TArray<ULevelStreaming*>& Levels);
	void PrepareLevel(const ULevel* Level, FLevelRecord& LevelRecord);
	void SerializeLevel(const ULevel* Level, const ULevelStreaming* StreamingLevel = nullptr);

	void SaveFile();
};
