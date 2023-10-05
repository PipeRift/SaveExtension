// Copyright 2015-2024 Piperift. All Rights Reserved.


#pragma once

#include "Serialization/LevelRecords.h"
#include "Serialization/Records.h"

#include <CoreMinimal.h>
#include <Engine/LevelScriptActor.h>
#include <Engine/LevelStreaming.h>
#include <GameFramework/SaveGame.h>

#include "SaveSlotData.generated.h"


/**
 * USaveSlotData stores all information that can be accessible only while the game is loaded.
 * Works like a common SaveGame object
 * E.g: Items, Quests, Enemies, World Actors, AI, Physics
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = SaveExtension, hideCategories = ("Activation", "Actor Tick", "Actor", "Input",
									   "Rendering", "Replication", "Socket", "Thumbnail"))
class SAVEEXTENSION_API USaveSlotData : public UObject
{
	GENERATED_BODY()

public:
	/** Game world time since game started in seconds */
	UPROPERTY(SaveGame, Category = SaveData, BlueprintReadOnly)
	float TimeSeconds;

	/** Records
	 * All serialized information to be saved or loaded
	 * Serialized manually for performance
	 */
	FObjectRecord GameInstance;
	TArray<FSubsystemRecord> GameInstanceSubsystems;

	TArray<FSubsystemRecord> WorldSubsystems;

	FPersistentLevelRecord RootLevel;
	TArray<FStreamingLevelRecord> SubLevels;


	void CleanRecords(bool bKeepSublevels);

	/** Using manual serialization. It's way faster than reflection serialization */
	virtual void Serialize(FArchive& Ar) override;
};
