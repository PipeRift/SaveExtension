// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "ISaveExtension.h"

#include <Engine/Level.h>
#include <Engine/LevelStreaming.h>
#include <GameFramework/Actor.h>
#include <Engine/LevelScriptActor.h>
#include <GameFramework/Controller.h>
#include <AIController.h>

#include "SavePreset.h"
#include "SlotData.h"

#include "SlotDataTask.h"
#include "SlotDataTask_Saver.generated.h"


/**
* Manages the saving process of a SaveData file
*/
UCLASS()
class USlotDataTask_Saver : public USlotDataTask
{
	GENERATED_BODY()

	int32 Slot;
	bool bOverride;

protected:

	// Async variables
	TWeakObjectPtr<ULevel> CurrentLevel;
	TWeakObjectPtr<ULevelStreaming> CurrentSLevel;

	int32 CurrentActorIndex;
	TArray<TWeakObjectPtr<AActor>> CurrentLevelActors;


public:

	auto Setup(int32 InSlot, bool bInOverride)
	{
		Slot = InSlot;
		bOverride = bInOverride;
		return this;
	}

	virtual void OnStart() override;

protected:

	/** BEGIN Serialization */
	void SerializeSync();
	void SerializeLevelSync(const ULevel* Level, const ULevelStreaming* StreamingLevel = nullptr);

	void SerializeASync();
	void SerializeLevelASync(const ULevel* Level, const ULevelStreaming* StreamingLevel = nullptr);

	/** Serializes all world actors. */
	void SerializeWorld();


private:

	void SerializeLevelScript(const ALevelScriptActor* Level, FLevelRecord& LevelRecord);

	void SerializeAI(const AAIController* AIController, FLevelRecord& LevelRecord);

	/** Serializes the GameMode. Only with 'SaveGameMode' enabled. */
	void SerializeGameMode();

	/** Serializes the GameState. Only with 'SaveGameState' enabled. */
	void SerializeGameState();

	/** Serializes the PlayerState. Only with 'SavePlayerState' enabled. */
	void SerializePlayerState(int32 PlayerId);

	/** Serializes PlayerControllers. Only with 'SavePlayerControllers' enabled. */
	void SerializePlayerController(int32 PlayerId);

	/** Serializes Player HUD Actor and its Properties.
	Requires 'SaveGameMode' flag to be used. */
	void SerializePlayerHUD(int32 PlayerId);

	/** Serializes Current Player's Pawn and its Properties.
	Requires 'SaveGameMode' flag to be used. */
	//void SerializePlayerPawn(int32 PlayerId);

	/** Serializes Game Instance Object and its Properties.
	Requires 'SaveGameMode' flag to be used. */
	void SerializeGameInstance();


	/** Serializes an actor into this Actor Record */
	bool SerializeActor(const AActor* Actor, FActorRecord& Record);

	/** Serializes an actor into this Controller Record */
	bool SerializeController(const AController* Actor, FControllerRecord& Record);

	/** Serializes the components of an actor into a provided Actor Record */
	void SerializeActorComponents(const AActor* Actor, FActorRecord& ActorRecord, int8 indent = 0);
	/** END Serialization */

	/** BEGIN FileSaving */
	bool SaveFile(const FString& InfoName, const FString& DataName) const;
	/** End FileSaving */
};
