// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "SaveSlotData.h"

#include <CoreMinimal.h>
#include <Engine/Texture2D.h>
#include <GameFramework/SaveGame.h>

#include "SaveSlot.generated.h"

struct FSELevelFilter;


/**
 * Specifies the behavior while saving or loading
 */
UENUM()
enum class ESaveASyncMode : uint8
{
	OnlySync,
	LoadAsync,
	SaveAsync,
	SaveAndLoadAsync
};


USTRUCT(Blueprintable)
struct FSaveSlotStats
{
	GENERATED_BODY()

	/** Played time since this saved game was started. Not related to slots, slots can change */
	UPROPERTY(BlueprintReadOnly, Category = Slot)
	FTimespan PlayedTime = FTimespan::Zero();

	/** Played time since this saved game was created */
	UPROPERTY(BlueprintReadOnly, Category = Slot)
	FTimespan SlotPlayedTime = FTimespan::Zero();

	/** Last date at which this slot was saved. */
	UPROPERTY(BlueprintReadOnly, Category = Slot)
	FDateTime SaveDate = FDateTime::Now();

	/** Date at which this slot was loaded. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = Slot)
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

public:
	/** Begin Settings */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	TSubclassOf<USaveSlotData> DataClass = USaveSlotData::StaticClass();

	/** Maximum amount of saved slots that use this class. 0 is infinite (~16000) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings", meta = (ClampMin = "0"))
	int32 MaxSlots = 0;

	/** If checked, will attempt to Save Game to first Slot found, timed event. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings")
	bool bPeriodicSave = false;

	/** Interval in seconds for auto saving */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings",
		meta = (EditCondition = "bPeriodicSave", UIMin = "15", UIMax = "3600"))
	int32 PeriodicSaveInterval = 120.f;

	/** If checked, will attempt to Save Game to current Slot found, timed event. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings")
	bool bSaveOnClose = false;

	/** If checked, will attempt to Load Game from last Slot found, when game starts */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings")
	bool bLoadOnStart = false;

	/** If true save files will be compressed
	 * Performance: Can add from 10ms to 20ms to loading and saving (estimate) but reduce file sizes making
	 * them up to 30x smaller
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Serialization")
	bool bUseCompression = true;

	/** If true will store the game instance */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Serialization)
	bool bStoreGameInstance = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Serialization")
	FSEActorClassFilter ActorFilter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Serialization",
		meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bUseLoadActorFilter = false;

	/** If enabled, this filter will be used while loading instead of "ActorFilter" */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Serialization",
		meta = (EditCondition = "bUseLoadActorFilter"))
	FSEActorClassFilter LoadActorFilter;

	/** If true will store ActorComponents depending on the filters */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Serialization")
	bool bStoreComponents = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Serialization")
	FSEComponentClassFilter ComponentFilter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Serialization",
		meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bUseLoadComponentFilter = false;

	/** If enabled, this filter will be used while loading instead of "ComponentFilter" */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Serialization",
		meta = (EditCondition = "bUseLoadComponentFilter"))
	FSEComponentClassFilter LoadComponentFilter;

	/** If true, will Save and Load levels when they are shown or hidden.
	 * This includes level streaming and world composition.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Level Streaming")
	bool bSaveAndLoadSublevels = true;

	/** Serialization will be multi-threaded between all available cores. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Async")
	ESaveASyncMode MultithreadedSerialization = ESaveASyncMode::SaveAsync;

	/** Split serialization between multiple frames. Ignored if MultithreadedSerialization is used
	 * Currently only implemented on Loading
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Async")
	ESaveASyncMode FrameSplittedSerialization = ESaveASyncMode::OnlySync;

	/** Max milliseconds to use every frame in an asynchronous operation.
	 * If running at 60Fps (16.6ms), loading or saving asynchronously will add MaxFrameMS:
	 * 16.6ms + 5MS = 21.6ms -> 46Fps
	 * This means gameplay will not be stopped nor have frame drops while saving or loading. Works best for
	 * non multi-threaded platforms
	 */
	UPROPERTY(
		EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Async", meta = (UIMin = "3", UIMax = "10"))
	float MaxFrameMs = 5.f;

	/** Files will be loaded or saved on a secondary thread while game continues */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Async")
	ESaveASyncMode MultithreadedFiles = ESaveASyncMode::SaveAndLoadAsync;

	/**
	 * If checked, will print messages to Log, and Viewport if DebugInScreen is enabled.
	 * Ignored in package or Shipping mode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Debug", AdvancedDisplay)
	bool bDebug = false;

	/**
	 * If checked and Debug is enabled, will print messages to Viewport.
	 * Ignored in package or Shipping mode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Debug", AdvancedDisplay,
		meta = (EditCondition = "bDebug"))
	bool bDebugInScreen = true;

	/** End Settings */


public:
	/** Slot where this SaveInfo and its saveData are saved */
	UPROPERTY(BlueprintReadWrite, Category = Slot)
	FName FileName = TEXT("Default");

	UPROPERTY(BlueprintReadWrite, Category = Slot)
	FText DisplayName;

	/** Root Level where this Slot was saved */
	UPROPERTY(BlueprintReadOnly, Category = Slot)
	FName Map;

	UPROPERTY(BlueprintReadWrite, Category = Slot)
	FSaveSlotStats Stats;

protected:
	UPROPERTY()
	FString ThumbnailPath;

	/** Thumbnail gets cached here the first time it is requested */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CachedThumbnail;

	UPROPERTY(BlueprintReadOnly, Transient, Category = Slot)
	TObjectPtr<USaveSlotData> Data;


public:
	USaveSlot();


	/** Returns this slot's thumbnail if any */
	UFUNCTION(BlueprintCallable, Category = Slot)
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

	UFUNCTION(BlueprintPure, Category = Slot)
	int32 GetMaxIndexes() const;

	UFUNCTION(BlueprintPure, Category = Slot)
	bool IsValidIndex(int32 Index) const;

	/** Internal Usage. Will be called when an screenshot is captured */
	void _SetThumbnailPath(const FString& Path);

	/** Internal Usage. Will be called to remove previous thumbnail */
	FString _GetThumbnailPath()
	{
		return ThumbnailPath;
	}

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Slot)
	bool SetIndex(int32 Index);

	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = Slot)
	int32 GetIndex() const;

protected:
	virtual bool OnSetIndex(int32 Index);
	virtual int32 OnGetIndex() const;

public:
	UFUNCTION(BlueprintPure, Category = SaveSlot)
	const FSEActorClassFilter& GetActorFilter(bool bIsLoading) const;

	UFUNCTION(BlueprintPure, Category = SaveSlot)
	const FSEComponentClassFilter& GetComponentFilter(bool bIsLoading) const;

	bool IsMTSerializationLoad() const;
	bool IsMTSerializationSave() const;

	ESaveASyncMode GetFrameSplitSerialization() const;
	float GetMaxFrameMs() const;

	bool IsFrameSplitLoad() const;
	bool IsFrameSplitSave() const;

	bool IsMTFilesLoad() const;
	bool IsMTFilesSave() const;

	FSELevelFilter ToFilter() const;
};
