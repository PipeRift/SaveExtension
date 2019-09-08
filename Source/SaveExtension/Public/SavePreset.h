// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/NoExportTypes.h"

#include "Misc/ClassFilter.h"
#include "Serialization/Serializer.h"
#include "SavePreset.generated.h"


/**
* Specifies the behavior while saving or loading
*/
UENUM()
enum class ESaveASyncMode : uint8 {
	OnlySync,
	LoadAsync,
	SaveAsync,
	SaveAndLoadAsync
};

class USlotInfo;
class USlotData;

/**
 * What to save, how to save it, when, every x minutes, what info file, what data file, save by level streaming?
 */
UCLASS(ClassGroup = SaveExtension, BlueprintType, Config = Game)
class SAVEEXTENSION_API USavePreset : public UDataAsset
{
	GENERATED_BODY()

public:

	USavePreset();


	/**
	* Specify the SaveInfo object to be used with this preset
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, Config)
	TSubclassOf<USlotInfo> SlotInfoTemplate;

	/**
	* Specify the SaveData object to be used with this preset
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, Config)
	TSubclassOf<USlotData> SlotDataTemplate;

	/** Maximum amount of saved slots at the same time */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay, Config, meta = (ClampMin = "0"))
	int32 MaxSlots;

	/** If checked, will attempt to Save Game to first Slot found, timed event. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, Config)
	bool bAutoSave;

	/** Interval in seconds for auto saving */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, Config, meta = (EditCondition = "bAutoSave", UIMin = "15", UIMax = "3600"))
	int32 AutoSaveInterval;

	/** If checked, will attempt to Save Game to first Slot found, timed event. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, Config, meta = (EditCondition = "bAutoSave"))
	bool bSaveOnExit;

	/** If checked, will attempt to Load Game from last Slot found, when game starts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, Config)
	bool bAutoLoad;

	/**
	 * If checked, will print messages to Log, and Viewport if DebugInScreen is enabled.
	 * Ignored in package or Shipping mode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, Config, AdvancedDisplay)
	bool bDebug;

	/**
	 * If checked and Debug is enabled, will print messages to Viewport.
	 * Ignored in package or Shipping mode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, Config, AdvancedDisplay, meta = (EditCondition = "bDebug"))
	bool bDebugInScreen;



	/** If true save files will be compressed
	 * Performance: Can add from 10ms to 20ms to loading and saving (estimate) but reduce file sizes making them up to 30x smaller
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Serialization, Config)
	bool bUseCompression;

	/** If true will store the game instance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Serialization, Config)
	bool bStoreGameInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization|Actors", Config)
	FActorClassFilter ActorFilter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization|Actors", Config, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bUseLoadActorFilter;

	/** If enabled, this filter will be used while loading instead of "ActorFilter" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization|Actors", Config, meta = (EditCondition="bUseLoadActorFilter"))
	FActorClassFilter LoadActorFilter;

	/** If true will store ActorComponents depending on the filters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization|Components", Config)
	bool bStoreComponents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization|Components", Config)
	FComponentClassFilter ComponentFilter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization|Components", Config, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bUseLoadComponentFilter;

	/** If enabled, this filter will be used while loading instead of "ComponentFilter" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization|Components", Config, meta = (EditCondition = "bUseLoadComponentFilter"))
	FComponentClassFilter LoadComponentFilter;

	// List of serializers by class
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Serialization, Config)
	TMap<UClass*, TSubclassOf<USerializer>> Serializers;

protected:

	/** Serialization will be multi-threaded between all available cores. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asynchronous")
	ESaveASyncMode MultithreadedSerialization;

	/** Split serialization between multiple frames. Ignored if MultithreadedSerialization is used
	 * Currently only implemented on Loading
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asynchronous")
	ESaveASyncMode FrameSplittedSerialization;


	/** Max milliseconds to use every frame in an asynchronous operation.
	 * If running at 60Fps (16.6ms), loading or saving asynchronously will add MaxFrameMS:
	 * 16.6ms + 5MS = 21.6ms -> 46Fps
	 * This means gameplay will not be stopped nor have frame drops while saving or loading. Works best for non multi-threaded platforms
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asynchronous", meta = (UIMin="3", UIMax="10"))
	float MaxFrameMs;

	/** Files will be loaded or saved on a secondary thread while game continues */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asynchronous")
	ESaveASyncMode MultithreadedFiles;


protected:

	/** If true, will Save and Load levels when they are shown or hidden.
	 * This includes level streaming and world composition.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Streaming")
	bool bSaveAndLoadSublevels;


	/** HELPERS */
public:

	FORCEINLINE int32 GetMaxSlots() const {
		return MaxSlots <= 0 ? 16384 : MaxSlots;
	}

	UFUNCTION(BlueprintPure, Category = SavePreset)
	FORCEINLINE FActorClassFilter& GetActorFilter(bool bIsLoading) {
		return (bIsLoading && bUseLoadActorFilter)? LoadActorFilter : ActorFilter;
	}

	FORCEINLINE const FActorClassFilter& GetActorFilter(bool bIsLoading) const {
		return (bIsLoading && bUseLoadActorFilter)? LoadActorFilter : ActorFilter;
	}

	UFUNCTION(BlueprintPure, Category = SavePreset)
	FORCEINLINE FComponentClassFilter& GetComponentFilter(bool bIsLoading) {
		return (bIsLoading && bUseLoadComponentFilter) ? LoadComponentFilter : ComponentFilter;
	}

	FORCEINLINE const FComponentClassFilter& GetComponentFilter(bool bIsLoading) const {
		return (bIsLoading && bUseLoadActorFilter) ? LoadComponentFilter : ComponentFilter;
	}

	FORCEINLINE bool IsMTSerializationLoad() const {
		return MultithreadedSerialization == ESaveASyncMode::LoadAsync || MultithreadedSerialization == ESaveASyncMode::SaveAndLoadAsync;
	}
	FORCEINLINE bool IsMTSerializationSave() const {
		return MultithreadedSerialization == ESaveASyncMode::SaveAsync || MultithreadedSerialization == ESaveASyncMode::SaveAndLoadAsync;
	}

	FORCEINLINE ESaveASyncMode GetFrameSplitSerialization() const { return FrameSplittedSerialization; }
	FORCEINLINE float GetMaxFrameMs() const { return MaxFrameMs; }

	FORCEINLINE bool IsFrameSplitLoad() const {
		return !IsMTSerializationLoad() && (FrameSplittedSerialization == ESaveASyncMode::LoadAsync || FrameSplittedSerialization == ESaveASyncMode::SaveAndLoadAsync);
	}
	FORCEINLINE bool IsFrameSplitSave() const {
		return !IsMTSerializationSave() && (FrameSplittedSerialization == ESaveASyncMode::SaveAsync || FrameSplittedSerialization == ESaveASyncMode::SaveAndLoadAsync);
	}

	FORCEINLINE bool IsMTFilesLoad() const {
		return MultithreadedFiles == ESaveASyncMode::LoadAsync || MultithreadedFiles == ESaveASyncMode::SaveAndLoadAsync;
	}
	FORCEINLINE bool IsMTFilesSave() const {
		return MultithreadedFiles == ESaveASyncMode::SaveAsync || MultithreadedFiles == ESaveASyncMode::SaveAndLoadAsync;
	}

};
