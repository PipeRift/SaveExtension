// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "SaveSlotData.h"

#include <CoreMinimal.h>
#include <Engine/Texture2D.h>
#include <GameFramework/SaveGame.h>

#include "SaveSlot.generated.h"


USTRUCT(Blueprintable)
struct FSaveSlotStats
{
	GENERATED_BODY()

	/** Played time since this saved game was started. Not related to slots, slots can change */
	UPROPERTY(BlueprintReadOnly, Category = SlotInfo)
	FTimespan PlayedTime = FTimespan::Zero();

	/** Played time since this saved game was created */
	UPROPERTY(BlueprintReadOnly, Category = SlotInfo)
	FTimespan SlotPlayedTime = FTimespan::Zero();

	/** Last date at which this slot was saved. */
	UPROPERTY(BlueprintReadOnly, Category = SlotInfo)
	FDateTime SaveDate = FDateTime::Now();

	/** Date at which this slot was loaded. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = SlotInfo)
	FDateTime LoadDate;
};


/**
 * USaveSlot stores information that needs to be accessible WITHOUT loading the game.
 * Works like a common SaveGame object
 * E.g: Dates, played time, progress, level
 */
UCLASS(ClassGroup = SaveExtension, hideCategories = ("Activation", "Actor Tick", "Actor", "Input",
									   "Rendering", "Replication", "Socket", "Thumbnail"))
class SAVEEXTENSION_API USaveSlot : public USaveGame
{
	GENERATED_BODY()

protected:
	/** Begin Settings */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SlotInfo|Settings")
	TSubclassOf<USaveSlotData> DataClass = USaveSlotData::StaticClass();

	/** Maximum amount of saved slots that use this class. 0 is infinite (~16000) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SlotInfo|Settings", meta = (ClampMin = "0"))
	int32 MaxSlots = 0;

	/** If checked, will attempt to Save Game to first Slot found, timed event. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlotInfo|Settings")
	bool bPeriodicSave = true;

	/** Interval in seconds for auto saving */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlotInfo|Settings",
		meta = (EditCondition = "bPeriodicSave", UIMin = "15", UIMax = "3600"))
	int32 PeriodicSaveInterval = 120.f;

	/** If checked, will attempt to Save Game to current Slot found, timed event. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlotInfo|Settings")
	bool bSaveOnClose = false;

	/** If checked, will attempt to Load Game from last Slot found, when game starts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlotInfo|Settings")
	bool bLoadOnStart = true;
	/** End Settings */


public:
	/** Slot where this SaveInfo and its saveData are saved */
	UPROPERTY(BlueprintReadWrite, Category = SlotInfo)
	FName FileName = TEXT("Default");

	UPROPERTY(BlueprintReadWrite, Category = SlotInfo)
	FText DisplayName;

	/** Root Level where this Slot was saved */
	UPROPERTY(BlueprintReadOnly, Category = SlotInfo)
	FName Map;

	UPROPERTY(BlueprintReadWrite, Category = SlotInfo)
	FSaveSlotStats Stats;

protected:
	UPROPERTY()
	FString ThumbnailPath;

	/** Thumbnail gets cached here the first time it is requested */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CachedThumbnail;

	UPROPERTY(BlueprintReadOnly, Transient, Category = SlotInfo)
	TObjectPtr<USaveSlotData> Data;


public:
	USaveSlot();


	/** Returns this slot's thumbnail if any */
	UFUNCTION(BlueprintCallable, Category = SlotInfo)
	UTexture2D* GetThumbnail() const;

	/** Captures a thumbnail for the current slot */
	bool CaptureThumbnail(const int32 Width = 640, const int32 Height = 360);


	USaveSlotData* GetData() const
	{
		return Data;
	}

	// Internal use only recommended
	void AssignData(USaveSlotData* NewData)
	{
		Data = NewData;
	}

	UFUNCTION(BlueprintPure, Category = SlotInfo)
	int32 GetMaxIds() const;

	UFUNCTION(BlueprintPure, Category = SlotInfo)
	bool IsValidId(int32 CheckedId) const;

	/** Internal Usage. Will be called when an screenshot is captured */
	void _SetThumbnailPath(const FString& Path);

	/** Internal Usage. Will be called to remove previous thumbnail */
	FString _GetThumbnailPath()
	{
		return ThumbnailPath;
	}

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = SlotInfo)
	bool SetId(int32 Id);

	UFUNCTION(BlueprintPure, BlueprintImplementableEvent, Category = SlotInfo)
	int32 GetId() const;

protected:
	virtual bool OnSetId(int32 Id);
	virtual int32 OnGetId() const;
};
