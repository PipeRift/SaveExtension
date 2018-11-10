// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <HAL/PlatformFilemanager.h>
#include <GenericPlatform/GenericPlatformFile.h>
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

#include "SaveManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameSaved,  USlotInfo*, SlotInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameLoaded, USlotInfo*, SlotInfo);


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
	/* PROPERTIES														   */
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


	UPROPERTY(Transient)
	TArray<ULevelStreamingNotifier*> LevelStreamingNotifiers;

	UPROPERTY(Transient)
	TArray<TScriptInterface<ISaveExtensionInterface>> SubscribedInterfaces;


	/************************************************************************/
	/* METHODS															  */
	/************************************************************************/
public:

	USaveManager();

	void Init();
	void Shutdown();

	void SetGameInstance(UGameInstance* GameInstance) { OwningGameInstance = GameInstance; }


	/** Save the Game into an specified Slot */
	UFUNCTION(Category = "SaveExtension|Saving", BlueprintCallable, meta = (AdvancedDisplay = 2))
	bool SaveSlotToId(int32 Slot = 0, bool bOverrideIfNeeded = true, bool bScreenshot = false, const int32 Width = 640, const int32 Height = 360);

	/** Save the Game to a Slot */
	UFUNCTION(Category = "SaveExtension|Saving", BlueprintCallable, meta = (AdvancedDisplay = 2))
	FORCEINLINE bool SaveSlot(const USlotInfo* SlotInfo, bool bOverrideIfNeeded = true, bool bScreenshot = false, const int32 Width = 640, const int32 Height = 360) {
		if (!SlotInfo) return false;
		return SaveSlotToId(SlotInfo->Id, bOverrideIfNeeded, bScreenshot, Width, Height);
	}

	/** Save the currently loaded Slot */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Saving", meta = (AdvancedDisplay = 1))
	bool SaveCurrentSlot(bool bScreenshot = false, const int32 Width = 640, const int32 Height = 360) {
		return SaveSlot(CurrentInfo, true, bScreenshot, Width, Height);
	}

	/** Load game from a slot Id */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading")
	bool LoadSlotFromId(int32 SlotId = 0);

	/** Load game from a SlotInfo */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading")
	FORCEINLINE bool LoadSlot(const USlotInfo* SlotInfo) {
		return SlotInfo ? LoadSlotFromId(SlotInfo->Id) : false;
	}

	/** Reload the currently loaded slot if any */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading")
	FORCEINLINE bool ReloadCurrentSlot() {
		return LoadSlot(CurrentInfo);
	}

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
	 * Performance: Interacts with disk, can be slow
	 * @param SlotId Id of the SlotInfo to be loaded
	 * @return the SlotInfo associated with an Id
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Slots")
	FORCEINLINE USlotInfo* GetSlotInfo(int32 SlotId) { return LoadInfo(SlotId); }

	/**
	 * Find all saved games and return their SlotInfos
	 * Performance: Interacts with disk, can be slow
	 * @param SaveInfos All saved games found on disk
	 * @param bSortByRecent Should slots be ordered by save date?
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Slots")
	void GetAllSlotInfos(TArray<USlotInfo*>& SaveInfos, const bool bSortByRecent = false);

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

	virtual FString GenerateSaveSlot(const int32 SlotId) const { return EventGenerateSaveSlot(SlotId); }

	FString GenerateSaveSlotName(const int32 SlotId) const {
		return GenerateSaveSlot(SlotId);
	}

	FString GenerateSaveDataSlotName(const int32 SlotId) const {
		return GenerateSaveSlotName(SlotId).Append(TEXT("_data"));
	}

protected:

	UFUNCTION(BlueprintNativeEvent, Category = SaveExtension, meta = (DisplayName = "Generate Save Slot"))
	FString EventGenerateSaveSlot(const int32 SlotId) const;

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

	USlotInfo* LoadInfoFromFile(const FString Name) const;

	void GetSlotFileNames(TArray<FString>& FoundFiles) const;

	void OnLevelLoaded(ULevelStreaming* StreamingLevel) {}


	//~ Begin Tasks
	UPROPERTY(Transient)
	TArray<USlotDataTask*> Tasks;

	USlotDataTask_Saver* CreateSaver();
	USlotDataTask_LevelSaver* CreateLevelSaver();
	USlotDataTask_Loader* CreateLoader();
	USlotDataTask_LevelLoader* CreateLevelLoader();
	USlotDataTask* CreateTask(UClass* TaskType);

	void FinishTask(USlotDataTask* Task);

	bool HasTasks() const { return Tasks.Num() > 0; }
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
	FOnGameSaved OnGameSaved;

	UPROPERTY(BlueprintAssignable, Category = SaveExtension)
	FOnGameLoaded OnGameLoaded;


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

	/** Used to find next available slot id */
	class FFindSlotVisitor : public IPlatformFile::FDirectoryVisitor
	{
	public:
		bool bOnlyInfos = false;
		bool bOnlyDatas = false;
		TArray<FString>& FilesFound;

		FFindSlotVisitor(TArray<FString>& Files)
			: FilesFound(Files)
		{}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override;
	};

	static TMap<TWeakObjectPtr<UGameInstance>, TWeakObjectPtr<USaveManager>> GlobalManagers;

public:

	/** Get the global save manager */
	UFUNCTION(BlueprintPure, Category = SaveExtension, meta = (WorldContext = "ContextObject"))
	static USaveManager* GetSaveManager(const UObject* ContextObject);
};
