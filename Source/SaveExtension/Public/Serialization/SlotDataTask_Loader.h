// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include "Delegates.h"
#include "ISaveExtension.h"
#include "Multithreading/LoadFileTask.h"
#include "SavePreset.h"
#include "SlotInfo.h"
#include "SlotData.h"
#include "SlotDataTask.h"

#include <Engine/Level.h>
#include <Engine/LevelStreaming.h>
#include <GameFramework/Actor.h>
#include <Engine/LevelScriptActor.h>
#include <GameFramework/Controller.h>

#include "SlotDataTask_Loader.generated.h"


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
UCLASS()
class USlotDataTask_Loader : public USlotDataTask
{
	GENERATED_BODY()

	int32 Slot;

	UPROPERTY()
	USlotInfo* NewSlotInfo;

	FOnGameLoaded Delegate;

protected:

	// Async variables
	TWeakObjectPtr<ULevel> CurrentLevel;
	TWeakObjectPtr<ULevelStreaming> CurrentSLevel;

	int32 CurrentActorIndex = 0;
	TArray<TWeakObjectPtr<AActor>> CurrentLevelActors;

	/** Start AsyncTasks */
	FAsyncTask<FLoadFileTask>* LoadDataTask;
	/** End AsyncTasks */

	ELoadDataTaskState LoadState = ELoadDataTaskState::NotStarted;


public:

	USlotDataTask_Loader() : Super() {}

	auto Setup(int32 InSlot)
	{
		Slot = InSlot;
		return this;
	}

	auto Bind(const FOnGameLoaded& OnLoaded) { Delegate = OnLoaded; return this; }

	void OnMapLoaded();

private:

	virtual void OnStart() override;

	virtual void Tick(float DeltaTime) override;
	virtual void OnFinish(bool bSuccess) override;
	virtual void BeginDestroy() override;

	void StartDeserialization();

	/** Spawns Actors hat were saved but which actors are not in the world. */
	void RespawnActors(const TArray<FActorRecord*>& Records, const ULevel* Level);

protected:

	//~ Begin Files
	void StartLoadingData();

	USlotData* GetLoadedData() const;
	FORCEINLINE const bool IsDataLoaded() const { return LoadDataTask && LoadDataTask->IsDone(); };
	//~ End Files


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

	/** Deserializes all Level actors. */
	inline void DeserializeLevel_Actor(AActor* const Actor, const FLevelRecord& LevelRecord);

private:

	/** Deserializes Game Instance Object and its Properties.
	Requires 'SaveGameMode' flag to be used. */
	void DeserializeGameInstance();

	/** Serializes an actor into this Actor Record */
	bool DeserializeActor(AActor* Actor, const FActorRecord& Record);

	/** Deserializes the components of an actor from a provided Record */
	void DeserializeActorComponents(AActor* Actor, const FActorRecord& ActorRecord, int8 indent = 0);
	/** END Deserialization */

protected:

	// HELPERS
	FLevelRecord* FindLevelRecord(const ULevelStreaming* Level) const;
	void FindNextAsyncLevel(ULevelStreaming*& OutLevelStreaming) const;
};