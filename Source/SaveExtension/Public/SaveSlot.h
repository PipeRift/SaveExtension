// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "SaveSlotData.h"

#include <CoreMinimal.h>
#include <GameFramework/SaveGame.h>

#include "SaveSlot.generated.h"

struct FSELevelFilter;
class UTexture2D;


DECLARE_DELEGATE_OneParam(FSEOnThumbnailCaptured, bool);

/**
 * Specifies the behavior while saving or loading
 */
UENUM()
enum class ESEAsyncMode : uint8
{
	SaveAndLoadSync = 0,
	LoadAsync = 1,
	SaveAsync = 2,
	SaveAndLoadAsync = LoadAsync | SaveAsync
};


USTRUCT(Blueprintable)
struct FSaveSlotStats
{
	GENERATED_BODY()

	/** Played time since this saved game was started. Not related to slots, slots can change */
	UPROPERTY(BlueprintReadOnly, Category = SaveSlot)
	FTimespan PlayedTime = FTimespan::Zero();

	/** Played time since this saved game was created */
	UPROPERTY(BlueprintReadOnly, Category = SaveSlot)
	FTimespan SlotPlayedTime = FTimespan::Zero();

	/** Last date at which this slot was saved. */
	UPROPERTY(BlueprintReadOnly, Category = SaveSlot)
	FDateTime SaveDate = FDateTime::Now();

	/** Date at which this slot was loaded. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = SaveSlot)
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Serialization")
	FSEComponentClassFilter ComponentFilter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Serialization")
	FSEClassFilter SubsystemFilter{ USubsystem::StaticClass() };

	/** If true, will Save and Load levels when they are shown or hidden.
	 * This includes level streaming and world composition.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Level Streaming")
	bool bSaveAndLoadSublevels = true;

	/** Serialization will be multi-threaded between all available cores. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Async")
	ESEAsyncMode MultithreadedSerialization = ESEAsyncMode::SaveAndLoadSync;

	/** Split serialization between multiple frames. Ignored if MultithreadedSerialization is used
	 * Currently only implemented on Loading
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Async")
	ESEAsyncMode FrameSplittedSerialization = ESEAsyncMode::SaveAndLoadSync;

	/** Max milliseconds to use every frame in an asynchronous operation.
	 * If running at 60Fps (16.6ms), loading or saving asynchronously will add MaxFrameMS:
	 * 16.6ms + 5MS = 21.6ms -> 46Fps
	 * This means gameplay will not be stopped nor have frame drops while saving or loading. Works best for
	 * non multi-threaded platforms
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Async", meta = (UIMin = "3", UIMax = "10"))
	float MaxFrameMs = 5.f;

	/** Files will be loaded or saved on a secondary thread while game continues */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Async")
	ESEAsyncMode MultithreadedFiles = ESEAsyncMode::SaveAndLoadAsync;

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
	UPROPERTY(BlueprintReadWrite, Category = SaveSlot)
	FName Name = TEXT("Default");

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = SaveSlot)
	FText DisplayName;

	/** Root Level where this Slot was saved */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = SaveSlot)
	FName Map;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = SaveSlot)
	FSaveSlotStats Stats;

	UPROPERTY(BlueprintReadWrite, Transient) // Saved
	TObjectPtr<UTexture2D> Thumbnail;

protected:

	UPROPERTY(Transient)
	bool bCapturingThumbnail = false;
	FSEOnThumbnailCaptured CapturedThumbnailDelegate;

	UPROPERTY(Transient, BlueprintReadOnly, Category = SaveSlot)
	TObjectPtr<USaveSlotData> Data;


public:
	void PostInitProperties() override;
	void Serialize(FArchive& Ar) override;

	/** Captures a thumbnail for the current slot
	 * @return true if thumbnail was requested. Only one can be requested at the same time.
	 */
	void CaptureThumbnail(FSEOnThumbnailCaptured Callback, const int32 Width = 640, const int32 Height = 360);

	/** Captures a thumbnail for the current slot */
	UFUNCTION(BlueprintCallable, Category = SaveSlot, meta = (DisplayName = "Capture Thumbnail"))
	void BPCaptureThumbnail(const int32 Width = 640, const int32 Height = 360)
	{
		CaptureThumbnail({}, Width, Height);
	}

	USaveSlotData* GetData() const
	{
		return Data;
	}

	// Internal use only recommended
	void AssignData(USaveSlotData* NewData)
	{
		Data = NewData;
	}

	bool ShouldDeserializeAsync() const;
	bool ShouldSerializeAsync() const;

	ESEAsyncMode GetFrameSplitSerialization() const;
	float GetMaxFrameMs() const;

	bool IsFrameSplitLoad() const;
	bool IsFrameSplitSave() const;

	bool ShouldLoadFileAsync() const;
	bool ShouldSaveFileAsync() const;

	UFUNCTION(BlueprintPure, Category = SaveSlot)
	bool IsLoadingOrSaving() const;

	// Called for every level before being saved or loaded
	UFUNCTION(BlueprintNativeEvent, Category = SaveSlot)
	void GetLevelFilter(bool bIsLoading, FSELevelFilter& OutFilter) const;

private:
	// Called for every level before being saved or loaded
	virtual void OnGetLevelFilter(bool bIsLoading, FSELevelFilter& OutFilter) const;

	void OnThumbnailCaptured(int32 InSizeX, int32 InSizeY, const TArray<FColor>& InImageData);
};
