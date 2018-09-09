// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SlotInfo.generated.h"

/**
 * USaveInfo is the savegame object in charge of saving basic and light info about a saved game.
 * E.g: Dates, played time, progress, level
 */
UCLASS(ClassGroup = SaveExtension, hideCategories = ("Activation", "Actor Tick", "Actor", "Input", "Rendering", "Replication", "Socket", "Thumbnail"))
class SAVEEXTENSION_API USlotInfo : public USaveGame
{
	GENERATED_UCLASS_BODY()

public:

	/** Slot where this SaveInfo and its saveData are saved */
	UPROPERTY(Category = SaveExtension, BlueprintReadOnly)
	int32 Id;

	/**
	 * Name of this slot
	 * Could be player written
	 */
	UPROPERTY(Category = SaveExtension, BlueprintReadWrite)
	FText Name;

	/**
	 * Subname of this slot
	 * Could be used as the name of the place we are saving from. E.g Frozen Lands
	 */
	UPROPERTY(Category = SaveExtension, BlueprintReadWrite)
	FText Subname;


	/** Played time since this saved game was started. Not related to slots, slots can change. */
	UPROPERTY(Category = SaveExtension, BlueprintReadOnly)
	FTimespan TotalPlayedTime;

	/** Played time since this slot was created. */
	UPROPERTY(Category = SaveExtension, BlueprintReadOnly)
	FTimespan SlotPlayedTime;

	/** Last date this slot was saved. */
	UPROPERTY(Category = SaveExtension, BlueprintReadOnly)
	FDateTime SaveDate;

	/** Route to the thumbnail. */
	UPROPERTY(Category = SaveExtension, BlueprintReadOnly)
	FString ThumbnailPath;

	/** Opened level when this Slot was saved. Streaming levels wont count, only root levels. */
	UPROPERTY(Category = SaveExtension, BlueprintReadOnly)
	FName Map;
};
