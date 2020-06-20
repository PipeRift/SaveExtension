// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "GameFramework/SaveGame.h"

#include "SlotInfo.generated.h"


/**
 * USaveInfo stores information that needs to be accessible WITHOUT loading the game.
 * Works like a common SaveGame object
 * E.g: Dates, played time, progress, level
 */
UCLASS(ClassGroup = SaveExtension, hideCategories = ("Activation", "Actor Tick", "Actor", "Input", "Rendering", "Replication", "Socket", "Thumbnail"))
class SAVEEXTENSION_API USlotInfo : public USaveGame
{
	GENERATED_BODY()

public:

	USlotInfo() : Super()
		, Id(0)
		, PlayedTime(FTimespan::Zero())
		, SlotPlayedTime(FTimespan::Zero())
		, SaveDate(FDateTime::Now())
	{}


	/** Slot where this SaveInfo and its saveData are saved */
	UPROPERTY(BlueprintReadOnly, Category = SlotInfo)
	int32 Id;

	UPROPERTY(BlueprintReadWrite, Category = SlotInfo)
	FText Name;

	/** Played time since this saved game was started. Not related to slots, slots can change */
	UPROPERTY(BlueprintReadOnly, Category = SlotInfo)
	FTimespan PlayedTime;

	/** Played time since this saved game was created */
	UPROPERTY(BlueprintReadOnly, Category = SlotInfo)
	FTimespan SlotPlayedTime;

	/** Last date this slot was saved. */
	UPROPERTY(BlueprintReadOnly, Category = SlotInfo)
	FDateTime SaveDate;

	/** Date at which this slot was loaded. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = SlotInfo)
	FDateTime LoadDate;

	/** Root Level where this Slot was saved */
	UPROPERTY(BlueprintReadOnly, Category = SlotInfo)
	FName Map;

private:

	UPROPERTY()
	FString ThumbnailPath;

	/** Thumbnail gets cached here the first time it is requested */
	UPROPERTY(Transient)
	UTexture2D* CachedThumbnail;


public:

	/** Returns this slot's thumbnail if any */
	UFUNCTION(BlueprintCallable, Category = SlotInfo)
	UTexture2D* GetThumbnail() const;

	/** Captures a thumbnail for the current slot */
	bool CaptureThumbnail(const int32 Width = 640, const int32 Height = 360);


	/** Internal Usage. Will be called when an screenshot is captured */
	void _SetThumbnailPath(const FString& Path);

	/** Internal Usage. Will be called to remove previous thumbnail */
	FString _GetThumbnailPath() { return ThumbnailPath; }
};
