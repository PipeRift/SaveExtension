// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <HAL/PlatformFilemanager.h>
#include <GenericPlatform/GenericPlatformFile.h>
#include <Async/AsyncWork.h>
#include <Engine/GameInstance.h>
#include <Tickable.h>

#include "SlotInfo.h"
#include "SlotData.h"

#include "LevelStreamingNotifier.h"
#include "SaveExtensionInterface.h"
#include "SavePreset.h"
#include "SlotDataTask.h"
#include "SlotDataTask_Saver.h"
#include "SlotDataTask_Loader.h"
#include "SlotDataTask_LevelSaver.h"
#include "SlotDataTask_LevelLoader.h"
#include "Multithreading/LoadAllSlotInfosTask.h"
#include "LatentActions/LoadGameAction.h"
#include "LatentActions/SaveGameAction.h"

#include "SaveManager.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameSavedMC,  USlotInfo*, SlotInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameLoadedMC, USlotInfo*, SlotInfo);


/**
 * Comment
 */
USTRUCT(BlueprintType)
struct FScreenshotSize
{
	GENERATED_BODY()

public:
	FScreenshotSize() : Width(640), Height(360) {}
	FScreenshotSize(int32 Width, int32 Height) : Width(Width), Height(Height) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Screenshot)
	int32 Width;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Screenshot)
	int32 Height;
};


/**
 *
 */
UCLASS(ClassGroup = SaveExtension, Config = Game)
class SAVEEXTENSION_API USaveManager : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

	friend USlotDataTask;
	friend USlotDataTask_Loader;


	/************************************************************************/
	/* PROPERTIES														    */
	/************************************************************************/
public:

	/** Settings Preset to be used while loading and saving. None is Default (Check Save Extension in Settings) */
	UPROPERTY(EditAnywhere, Category = "Save Extension", Config, meta = (DisplayName = "Preset"))
	TAssetPtr<USavePreset> PresetAsset;

private:

	/** Currently loaded SaveInfo. SaveInfo stores basic information about a saved game. Played time, levels, progress, etc. */
	UPROPERTY()
	USlotInfo* CurrentInfo;

	/** Currently loaded SaveData. SaveData stores all serialized info about the world. */
	UPROPERTY()
	USlotData* CurrentData;

	/** The game instance to which this save manager is owned. */
	TWeakObjectPtr<UGameInstance> OwningGameInstance;


	TArray<FAsyncTask<FLoadAllSlotInfosTask>*> LoadInfosTasks;

	UPROPERTY(Transient)
	TArray<ULevelStreamingNotifier*> LevelStreamingNotifiers;

	UPROPERTY(Transient)
	TArray<TScriptInterface<ISaveExtensionInterface>> SubscribedInterfaces;


	/************************************************************************/
	/* METHODS											     			    */
	/************************************************************************/
public:

	USaveManager();

	void Init();
	void Shutdown();

	void SetGameInstance(UGameInstance* GameInstance) { OwningGameInstance = GameInstance; }

	/** BLUEPRINTS */

	/** Save the Game into an specified Slot */
	bool SaveSlot(int32 SlotId, bool bOverrideIfNeeded = true, bool bScreenshot = false, const FScreenshotSize Size = {}, FOnGameSaved OnSaved = {});

	/** Save the Game to a Slot */
	bool SaveSlot(const USlotInfo* SlotInfo, bool bOverrideIfNeeded = true, bool bScreenshot = false, const FScreenshotSize Size = {}, FOnGameSaved OnSaved = {}) {
		if (!SlotInfo)
			return false;
		return SaveSlot(SlotInfo->Id, bOverrideIfNeeded, bScreenshot, Size, OnSaved);
	}

	/** Save the currently loaded Slot */
	bool SaveCurrentSlot(bool bScreenshot = false, const FScreenshotSize Size = {}, FOnGameSaved OnSaved = {}) {
		return SaveSlot(CurrentInfo, true, bScreenshot, Size, OnSaved);
	}


	/** Load game from a slot Id */
	bool LoadSlot(int32 SlotId, FOnGameLoaded OnLoaded = {});

	/** Load game from a SlotInfo */
	FORCEINLINE bool LoadSlot(const USlotInfo* SlotInfo, FOnGameLoaded OnLoaded = {}) {
		return SlotInfo ? LoadSlot(SlotInfo->Id, MoveTemp(OnLoaded)) : false;
	}

	/** Reload the currently loaded slot if any */
	FORCEINLINE bool ReloadCurrentSlot(FOnGameLoaded OnLoaded = {}) {
		return LoadSlot(CurrentInfo, MoveTemp(OnLoaded));
	}


	/**
	 * Find all saved games and return their SlotInfos
	 * @param bSortByRecent Should slots be ordered by save date?
	 * @param SaveInfos All saved games found on disk
	 */
	void LoadAllSlotInfos(bool bSortByRecent, FOnAllInfosLoaded Delegate);


private:

	/** BLUEPRINT ONLY API */

	// NOTE: This functions are mostly made to accommodate better Blueprint nodes and directly communicate with the normal C++ API

	/** Save the Game into an specified Slot */
	UFUNCTION(Category = "SaveExtension|Saving", BlueprintCallable, meta = (AdvancedDisplay = "bScreenshot, Size", DisplayName = "Save Slot to Id", Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPSaveSlotToId(int32 SlotId, bool bScreenshot, const FScreenshotSize Size, ESaveGameResult& Result, struct FLatentActionInfo LatentInfo, bool bOverrideIfNeeded = true);

	/** Save the Game to a Slot */
	UFUNCTION(Category = "SaveExtension|Saving", BlueprintCallable, meta = (AdvancedDisplay = "bScreenshot, Size", DisplayName = "Save Slot", Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPSaveSlot(const USlotInfo* SlotInfo, bool bScreenshot, const FScreenshotSize Size, ESaveGameResult& Result, struct FLatentActionInfo LatentInfo, bool bOverrideIfNeeded = true) {
		if (!SlotInfo)
		{
			Result = ESaveGameResult::Failed;
			return;
		}
		BPSaveSlotToId(SlotInfo->Id, bScreenshot, Size, Result, MoveTemp(LatentInfo), bOverrideIfNeeded);
	}

	/** Save the currently loaded Slot */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Saving", meta = (AdvancedDisplay = "bScreenshot, Size", DisplayName = "Save Current Slot", Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPSaveCurrentSlot(bool bScreenshot, const FScreenshotSize Size, ESaveGameResult& Result, struct FLatentActionInfo LatentInfo) {
		BPSaveSlot(CurrentInfo, bScreenshot, Size, Result, MoveTemp(LatentInfo), true);
	}


	/** Load game from a slot Id */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading", meta = (DisplayName = "Load Slot from Id", Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPLoadSlotFromId(int32 SlotId, ELoadGameResult& Result, struct FLatentActionInfo LatentInfo);

	/** Load game from a SlotInfo */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading", meta = (DisplayName = "Load Slot", Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPLoadSlot(const USlotInfo* SlotInfo, ELoadGameResult& Result, struct FLatentActionInfo LatentInfo) {
		if (!SlotInfo)
		{
			Result = ELoadGameResult::Failed;
			return;
		}
		BPLoadSlotFromId(SlotInfo->Id, Result, MoveTemp(LatentInfo));
	}

	/** Reload the currently loaded slot if any */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading", meta = (DisplayName = "Reload Current Slot", Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPReloadCurrentSlot(ELoadGameResult& Result, struct FLatentActionInfo LatentInfo) {
		BPLoadSlot(CurrentInfo, Result, MoveTemp(LatentInfo));
	}


	/**
	 * Find all saved games and return their SlotInfos
	 * @param bSortByRecent Should slots be ordered by save date?
	 * @param SaveInfos All saved games found on disk
	 */
	UFUNCTION(BlueprintCallable, Category = Dialogue, meta = (Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", Duration = "-1.0", DisplayName = "Load All Slot Infos"))
	void BPLoadAllSlotInfos(const bool bSortByRecent, TArray<USlotInfo*>& SaveInfos, ELoadInfoResult& Result, struct FLatentActionInfo LatentInfo);


public:

	/** BLUEPRINTS & C++ API */

	/** Delete a saved game on an specified slot Id
	 * Performance: Interacts with disk, can be slow
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension")
	bool DeleteSlot(int32 SlotId);


	/** Get the currently loaded SlotInfo. If game was never loaded returns a new SlotInfo */
	UFUNCTION(BlueprintPure, Category = "SaveExtension")
	FORCEINLINE USlotInfo* GetCurrentInfo() {
		TryInstantiateInfo();
		return CurrentInfo;
	}

	/** Get the currently loaded SlotData. If game was never loaded returns an empty SlotData  */
	UFUNCTION(BlueprintPure, Category = "SaveExtension")
	FORCEINLINE USlotData* GetCurrentData() {
		TryInstantiateInfo();
		return CurrentData;
	}

	/**
	 * Load and return an SlotInfo by Id if it exists
	 * Performance: Interacts with disk, could be slow if called frequently
	 * @param SlotId Id of the SlotInfo to be loaded
	 * @return the SlotInfo associated with an Id
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Slots")
	FORCEINLINE USlotInfo* GetSlotInfo(int32 SlotId) { return LoadInfo(SlotId); }

	/** Check if an slot exists on disk
	 * @return true if the slot exists
	 */
	UFUNCTION(BlueprintPure, Category = "SaveExtension|Slots")
	bool IsSlotSaved(int32 Slot) const;

	/** Check if currently playing in a saved slot
	 * @return true if currently playing in a saved slot
	 */
	UFUNCTION(BlueprintPure, Category = "SaveExtension|Slots")
	FORCEINLINE bool IsInSlot() const { return CurrentInfo && CurrentData; }

	/**
	 * Set the preset to be used for saving and loading
	 * @return true if the preset was set successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension")
	bool SetActivePreset(TAssetPtr<USavePreset> ActivePreset)
	{
		// We can only change a preset if we have no tasks running
		if (!HasTasks())
		{
			PresetAsset = ActivePreset;
			return true;
		}
		return false;
	}

	const USavePreset* GetPreset() const {
		if (!PresetAsset.IsNull())
		{
			return PresetAsset.LoadSynchronous();
		}
		return GetDefault<USavePreset>();
	}


	void TryInstantiateInfo(bool bForced = false);

	virtual FString GenerateBaseSlotName(const int32 SlotId) const {
		return IsValidSlot(SlotId)? FString::FromInt(SlotId) : FString{};
	}

	FString GenerateSlotInfoName(const int32 SlotId) const {
		return GenerateBaseSlotName(SlotId);
	}

	FString GenerateSlotDataName(const int32 SlotId) const {
		return GenerateSlotInfoName(SlotId).Append(TEXT("_data"));
	}

protected:

	bool CanLoadOrSave();

private:

	//~ Begin LevelStreaming
	void UpdateLevelStreamings();

	UFUNCTION()
	void SerializeStreamingLevel(ULevelStreaming* LevelStreaming);
	UFUNCTION()
	void DeserializeStreamingLevel(ULevelStreaming* LevelStreaming);
	//~ End LevelStreaming


	USlotInfo* LoadInfo(uint32 Slot) const;
	USlotData* LoadData(const USlotInfo* Info) const;

	FORCEINLINE bool IsValidSlot(const int32 Slot) const {
		const int32 MaxSlots = GetPreset()->GetMaxSlots();
		return Slot >= 0 && (MaxSlots <= 0 || Slot < MaxSlots);
	}

	void OnLevelLoaded(ULevelStreaming* StreamingLevel) {}


	//~ Begin Tasks
	UPROPERTY(Transient)
	TArray<USlotDataTask*> Tasks;

	template<class TaskType>
	TaskType* CreateTask() {
		return Cast<TaskType>(CreateTask(TaskType::StaticClass()));
	}
	USlotDataTask* CreateTask(TSubclassOf<USlotDataTask> TaskType);

	void FinishTask(USlotDataTask* Task);

	bool HasTasks() const { return Tasks.Num() > 0; }

	/** @return true when saving or loading anything, including levels */
	UFUNCTION(BlueprintPure, Category = SaveExtension)
	FORCEINLINE bool IsSavingOrLoading() const { return HasTasks(); }
	//~ End Tasks


	//~ Begin Tickable Object Interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override {
		return !IsDefaultSubobject() && !IsPendingKill();
	}
	virtual TStatId GetStatId() const override {
		RETURN_QUICK_DECLARE_CYCLE_STAT(USaveManager, STATGROUP_Tickables);
	}
	//~ End Tickable Object Interface


	/***********************************************************************/
	/* EVENTS															  */
	/***********************************************************************/
public:

	UPROPERTY(BlueprintAssignable, Category = SaveExtension)
	FOnGameSavedMC OnGameSaved;

	UPROPERTY(BlueprintAssignable, Category = SaveExtension)
	FOnGameLoadedMC OnGameLoaded;


	/** Subscribe to receive save and load events on an Interface */
	UFUNCTION(Category = SaveExtension, BlueprintCallable)
	void SubscribeForEvents(const TScriptInterface<ISaveExtensionInterface>& Interface);

	/** Unsubscribe to no longer receive save and load events on an Interface */
	UFUNCTION(Category = SaveExtension, BlueprintCallable)
	void UnsubscribeFromEvents(const TScriptInterface<ISaveExtensionInterface>& Interface);


	void OnSaveBegan();
	void OnSaveFinished(const bool bError);
	void OnLoadBegan();
	void OnLoadFinished(const bool bError);

private:

	void OnMapLoadStarted(const FString& MapName);
	void OnMapLoadFinished(UWorld* LoadedWorld);

	void IterateSubscribedInterfaces(TFunction<void(UObject*)>&& Callback) {
		for (const TScriptInterface<ISaveExtensionInterface>& Interface : SubscribedInterfaces)
		{
			if (UObject* const Object = Interface.GetObject())
				Callback(Object);
		}
	}

	//~ Begin UObject Interface
	virtual UWorld* GetWorld() const override;
	virtual void BeginDestroy() override;
	//~ End UObject Interface


	/**
	* STATIC
	*/

	static TMap<TWeakObjectPtr<UGameInstance>, TWeakObjectPtr<USaveManager>> GlobalManagers;

public:

	/** Get the global save manager */
	UFUNCTION(BlueprintPure, Category = SaveExtension, meta = (WorldContext = "ContextObject"))
	static USaveManager* GetSaveManager(const UObject* ContextObject);
};
