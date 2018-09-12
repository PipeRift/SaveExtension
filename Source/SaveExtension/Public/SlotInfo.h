// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
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
	UPROPERTY(Category = SlotInfo, BlueprintReadOnly)
	int32 Id;

	/**
	 * Name of this slot
	 * Could be player written
	 */
	UPROPERTY(Category = SlotInfo, BlueprintReadWrite)
	FText Name;

	/**
	 * Subname of this slot
	 * Could be used as the name of the place we are saving from. E.g Frozen Lands
	 */
	UPROPERTY(Category = SlotInfo, BlueprintReadWrite)
	FText Subname;


	/** Played time since this saved game was started. Not related to slots, slots can change. */
	UPROPERTY(Category = SlotInfo, BlueprintReadOnly)
	FTimespan TotalPlayedTime;

	/** Played time since this slot was created. */
	UPROPERTY(Category = SlotInfo, BlueprintReadOnly)
	FTimespan SlotPlayedTime;

	/** Last date this slot was saved. */
	UPROPERTY(Category = SlotInfo, BlueprintReadOnly)
	FDateTime SaveDate;

	/** Opened level when this Slot was saved. Streaming levels wont count, only root levels. */
	UPROPERTY(Category = SlotInfo, BlueprintReadOnly)
	FName Map;

private:

	/** Route to the thumbnail. */
	UPROPERTY()
	FString ThumbnailPath;

	UPROPERTY(Transient)
	UTexture2D * CachedThumbnail;

public:

	/** Returns a loaded thumbnail if any */
	UFUNCTION(BlueprintCallable, Category = SlotInfo)
	UTexture2D* GetThumbnail() const;

	/** Saves a thumbnail for the current slot */
	bool SaveThumbnail(const int32 Width = 640, const int32 Height = 360);

	/** Internal Usage. Will be called when an screenshot is captured */
	void SetThumbnailPath(const FString& Path);

	/** Internal Usage. Will be called to remove previous thumbnail */
	FString GetThumbnailPath() { return ThumbnailPath; }
};
