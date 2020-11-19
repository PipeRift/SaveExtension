// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/NoExportTypes.h"

#include "Misc/ClassFilter.h"
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
UCLASS(ClassGroup = SaveExtension, Blueprintable)
class SAVEEXTENSION_API USavePreset : public UObject
{
	GENERATED_BODY()

public:

	/**
	* Specify the SaveInfo object to be used with this preset
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	TSubclassOf<USlotInfo> SlotInfoClass;

	/**
	* Specify the SaveData object to be used with this preset
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	TSubclassOf<USlotData> SlotDataClass;

	/** Maximum amount of saved slots at the same time */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Gameplay, meta = (ClampMin = "0"))
	int32 MaxSlots = 0;

	/** If checked, will attempt to Save Game to first Slot found, timed event. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	bool bAutoSave = true;

	/** Interval in seconds for auto saving */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, meta = (EditCondition = "bAutoSave", UIMin = "15", UIMax = "3600"))
	int32 AutoSaveInterval = 120.f;

	/** If checked, will attempt to Save Game to first Slot found, timed event. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, meta = (EditCondition = "bAutoSave"))
	bool bSaveOnExit = false;

	/** If checked, will attempt to Load Game from last Slot found, when game starts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	bool bAutoLoad = true;

	/**
	 * If checked, will print messages to Log, and Viewport if DebugInScreen is enabled.
	 * Ignored in package or Shipping mode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, AdvancedDisplay)
	bool bDebug = false;

	/**
	 * If checked and Debug is enabled, will print messages to Viewport.
	 * Ignored in package or Shipping mode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, AdvancedDisplay, meta = (EditCondition = "bDebug"))
	bool bDebugInScreen = true;


	/** If true save files will be compressed
	 * Performance: Can add from 10ms to 20ms to loading and saving (estimate) but reduce file sizes making them up to 30x smaller
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Serialization)
	bool bUseCompression = true;

	/** If true will store the game instance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Serialization)
	bool bStoreGameInstance = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization|Actors")
	FSEActorClassFilter ActorFilter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization|Actors", meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bUseLoadActorFilter = false;

	/** If enabled, this filter will be used while loading instead of "ActorFilter" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization|Actors", meta = (EditCondition="bUseLoadActorFilter"))
	FSEActorClassFilter LoadActorFilter;

	/** If true will store ActorComponents depending on the filters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization|Components")
	bool bStoreComponents = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization|Components")
	FSEComponentClassFilter ComponentFilter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization|Components", meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bUseLoadComponentFilter = false;

	/** If enabled, this filter will be used while loading instead of "ComponentFilter" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization|Components", meta = (EditCondition = "bUseLoadComponentFilter"))
	FSEComponentClassFilter LoadComponentFilter;

public:

	/** Serialization will be multi-threaded between all available cores. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asynchronous")
	ESaveASyncMode MultithreadedSerialization = ESaveASyncMode::SaveAsync;

	/** Split serialization between multiple frames. Ignored if MultithreadedSerialization is used
	 * Currently only implemented on Loading
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asynchronous")
	ESaveASyncMode FrameSplittedSerialization = ESaveASyncMode::OnlySync;


	/** Max milliseconds to use every frame in an asynchronous operation.
	 * If running at 60Fps (16.6ms), loading or saving asynchronously will add MaxFrameMS:
	 * 16.6ms + 5MS = 21.6ms -> 46Fps
	 * This means gameplay will not be stopped nor have frame drops while saving or loading. Works best for non multi-threaded platforms
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asynchronous", meta = (UIMin="3", UIMax="10"))
	float MaxFrameMs = 5.f;

	/** Files will be loaded or saved on a secondary thread while game continues */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asynchronous")
	ESaveASyncMode MultithreadedFiles = ESaveASyncMode::SaveAndLoadAsync;


protected:

	/** If true, will Save and Load levels when they are shown or hidden.
	 * This includes level streaming and world composition.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level Streaming")
	bool bSaveAndLoadSublevels = true;


public:

	USavePreset();

	UFUNCTION(BlueprintNativeEvent, Category = "Slots", meta = (DisplayName="Generate Slot Name"))
	void BPGetSlotNameFromId(int32 Id, FName& Name) const;

protected:

	virtual void GetSlotNameFromId(int32 Id, FName& Name) const;


	/** HELPERS */
public:

	int32 GetMaxIds() const;

	bool IsValidId(int32 Id) const;

	UFUNCTION(BlueprintPure, Category = SavePreset)
	FSEActorClassFilter& GetActorFilter(bool bIsLoading)
	{
		return (bIsLoading && bUseLoadActorFilter)? LoadActorFilter : ActorFilter;
	}

	const FSEActorClassFilter& GetActorFilter(bool bIsLoading) const
	{
		return (bIsLoading && bUseLoadActorFilter)? LoadActorFilter : ActorFilter;
	}

	UFUNCTION(BlueprintPure, Category = SavePreset)
	FSEComponentClassFilter& GetComponentFilter(bool bIsLoading)
	{
		return (bIsLoading && bUseLoadComponentFilter) ? LoadComponentFilter : ComponentFilter;
	}

	const FSEComponentClassFilter& GetComponentFilter(bool bIsLoading) const
	{
		return (bIsLoading && bUseLoadActorFilter) ? LoadComponentFilter : ComponentFilter;
	}

	bool IsMTSerializationLoad() const
	{
		return MultithreadedSerialization == ESaveASyncMode::LoadAsync || MultithreadedSerialization == ESaveASyncMode::SaveAndLoadAsync;
	}
	bool IsMTSerializationSave() const
	{
		return MultithreadedSerialization == ESaveASyncMode::SaveAsync || MultithreadedSerialization == ESaveASyncMode::SaveAndLoadAsync;
	}

	ESaveASyncMode GetFrameSplitSerialization() const { return FrameSplittedSerialization; }
	float GetMaxFrameMs() const { return MaxFrameMs; }

	bool IsFrameSplitLoad() const
	{
		return !IsMTSerializationLoad() && (FrameSplittedSerialization == ESaveASyncMode::LoadAsync || FrameSplittedSerialization == ESaveASyncMode::SaveAndLoadAsync);
	}
	bool IsFrameSplitSave() const
	{
		return !IsMTSerializationSave() && (FrameSplittedSerialization == ESaveASyncMode::SaveAsync || FrameSplittedSerialization == ESaveASyncMode::SaveAndLoadAsync);
	}

	bool IsMTFilesLoad() const
	{
		return MultithreadedFiles == ESaveASyncMode::LoadAsync || MultithreadedFiles == ESaveASyncMode::SaveAndLoadAsync;
	}
	bool IsMTFilesSave() const
	{
		return MultithreadedFiles == ESaveASyncMode::SaveAsync || MultithreadedFiles == ESaveASyncMode::SaveAndLoadAsync;
	}

	struct FSaveFilter ToFilter() const;
};


inline int32 USavePreset::GetMaxIds() const
{
	return MaxSlots <= 0 ? 16384 : MaxSlots;
}

inline bool USavePreset::IsValidId(int32 Id) const
{
	const int32 MaxIds = GetMaxIds();
	return Id >= 0 && (MaxIds <= 0 || Id < MaxIds);
}
