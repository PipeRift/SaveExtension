// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "Delegates.h"
#include "ISaveExtension.h"
#include "SaveSlot.h"
#include "SaveSlotData.h"
#include "SEDataTask.h"

#include <Engine/Level.h>
#include <Engine/LevelScriptActor.h>
#include <Engine/LevelStreaming.h>
#include <GameFramework/Actor.h>
#include <GameFramework/Controller.h>


enum class ELoadDataTaskState : uint8
{
	NotStarted,

	// Once loading starts we either load the map
	LoadingMap,
	WaitingForData,

	RestoringActors,
	Deserializing
};

/**
 * Manages the loading process of a SaveData file
 */
struct FSEDataTask_Load : public FSEDataTask
{
protected:
	FName SlotName;

	TObjectPtr<USaveSlot> Slot;

	FOnGameLoaded Delegate;

protected:
	// Async variables
	TWeakObjectPtr<ULevel> CurrentLevel;
	TWeakObjectPtr<ULevelStreaming> CurrentSLevel;

	int32 CurrentActorIndex = 0;
	TArray<TWeakObjectPtr<AActor>> CurrentLevelActors;

	UE::Tasks::TTask<USaveSlot*> LoadFileTask;

	ELoadDataTaskState LoadState = ELoadDataTaskState::NotStarted;


public:
	FSEDataTask_Load(USaveManager* Manager, USaveSlot* Slot)
		: FSEDataTask(Manager, Slot, ESETaskType::Load)
	{}
	~FSEDataTask_Load();

	auto& Setup(FName InSlotName)
	{
		SlotName = InSlotName;
		return *this;
	}

	auto& Bind(const FOnGameLoaded& OnLoaded)
	{
		Delegate = OnLoaded;
		return *this;
	}

	void OnMapLoaded();

private:
	virtual void OnStart() override;

	virtual void Tick(float DeltaTime) override;
	virtual void OnFinish(bool bSuccess) override;

	void StartDeserialization();

	/** Spawns Actors hat were saved but which actors are not in the world. */
	void RespawnActors(const TArray<FActorRecord*>& Records, const ULevel* Level, FLevelRecord& LevelRecord);

protected:
	void StartLoadingFile();
	bool CheckFileLoaded();

	/** BEGIN Deserialization */
	void BeforeDeserialize();
	void DeserializeSync();
	void DeserializeLevelSync(const ULevel* Level, const ULevelStreaming* StreamingLevel = nullptr);

	void DeserializeASync();
	void DeserializeLevelASync(ULevel* Level, ULevelStreaming* StreamingLevel = nullptr);

	virtual void DeserializeASyncLoop(float StartMS = 0.0f);

	void FinishedDeserializing();

	void PrepareAllLevels();
	void PrepareLevel(const ULevel* Level, FLevelRecord& LevelRecord);

	void FindNextAsyncLevel(ULevelStreaming*& OutLevelStreaming) const;

	/** Deserializes Game Instance Object and its Properties.
	 * Requires 'SaveGameInstance' flag to be used.
	 */
	void DeserializeGameInstance();

	/** Serializes an actor into this Actor Record */
	bool DeserializeActor(AActor* Actor, const FActorRecord& ActorRecord, const FLevelRecord& LevelRecord);

	/** Deserializes the components of an actor from a provided Record */
	void DeserializeActorComponents(AActor* Actor, const FActorRecord& ActorRecord, const FLevelRecord& LevelRecord, int8 indent = 0);
	/** END Deserialization */
};