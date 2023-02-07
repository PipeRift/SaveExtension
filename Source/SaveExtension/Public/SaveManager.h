// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include "Delegates.h"
#include "LatentActions/DeleteSlotsAction.h"
#include "LatentActions/LoadGameAction.h"
#include "LatentActions/SaveGameAction.h"
#include "LevelStreamingNotifier.h"
#include "Multithreading/ScopedTaskManager.h"
#include "Multithreading/Delegates.h"
#include "SaveExtensionInterface.h"
#include "SavePreset.h"
#include "Serialization/SlotDataTask.h"
#include "SlotData.h"
#include "SlotInfo.h"

#include <Async/AsyncWork.h>
#include <CoreMinimal.h>
#include <Engine/GameInstance.h>
#include <GenericPlatform/GenericPlatformFile.h>
#include <HAL/PlatformFilemanager.h>
#include <Subsystems/GameInstanceSubsystem.h>
#include <Tickable.h>

#include "SaveManager.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameSavedMC, USlotInfo*, SlotInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameLoadedMC, USlotInfo*, SlotInfo);


struct FLatentActionInfo;

USTRUCT(BlueprintType)
struct FScreenshotSize
{
	GENERATED_BODY()

public:
	FScreenshotSize() : Width(640), Height(360) {}
	FScreenshotSize(int32 InWidth, int32 InHeight) : Width(InWidth), Height(InHeight) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Screenshot)
	int32 Width;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Screenshot)
	int32 Height;
};


/**
 * Controls the complete saving and loading process
 */
UCLASS(ClassGroup = SaveExtension, meta = (DisplayName = "SaveManager"))
class SAVEEXTENSION_API USaveManager : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

	friend USlotDataTask;


	/************************************************************************/
	/* PROPERTIES														    */
	/************************************************************************/
public:

	// Loaded from settings. Can be changed at runtime
	UPROPERTY(Transient, BlueprintReadWrite, Category=SaveManager)
	bool bTickWithGameWorld = false;

private:
	UPROPERTY(Transient)
	USavePreset* ActivePreset;

	/** Currently loaded SaveInfo. SaveInfo stores basic information about a saved game. Played time, levels,
	 * progress, etc. */
	UPROPERTY()
	USlotInfo* CurrentInfo;

	/** Currently loaded SaveData. SaveData stores all serialized info about the world. */
	UPROPERTY()
	USlotData* CurrentData;

	/** The game instance to which this save manager is owned. */
	TWeakObjectPtr<UGameInstance> OwningGameInstance;

	FScopedTaskList MTTasks;

	UPROPERTY(Transient)
	TArray<ULevelStreamingNotifier*> LevelStreamingNotifiers;

	UPROPERTY(Transient)
	TArray<TScriptInterface<ISaveExtensionInterface>> SubscribedInterfaces;

	UPROPERTY(Transient)
	TArray<USlotDataTask*> Tasks;


	/************************************************************************/
	/* METHODS											     			    */
	/************************************************************************/
public:
	USaveManager();


	/** Begin USubsystem */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;
	/** End USubsystem */

	void SetGameInstance(UGameInstance* GameInstance)
	{
		OwningGameInstance = GameInstance;
	}

	/** C++ ONLY API */

	/** Save the Game into an specified slot name */
	bool SaveSlot(FName SlotName, bool bOverrideIfNeeded = true, bool bScreenshot = false,
		const FScreenshotSize Size = {}, FOnGameSaved OnSaved = {});

	/** Save the Game info an SlotInfo */
	bool SaveSlot(const USlotInfo* SlotInfo, bool bOverrideIfNeeded = true, bool bScreenshot = false,
		const FScreenshotSize Size = {}, FOnGameSaved OnSaved = {});

	/** Save the Game into an specified slot id */
	bool SaveSlot(int32 SlotId, bool bOverrideIfNeeded = true, bool bScreenshot = false,
		const FScreenshotSize Size = {}, FOnGameSaved OnSaved = {});

	/** Save the currently loaded Slot */
	bool SaveCurrentSlot(bool bScreenshot = false, const FScreenshotSize Size = {}, FOnGameSaved OnSaved = {});


	/** Load game from a file name */
	bool LoadSlot(FName SlotName, FOnGameLoaded OnLoaded = {});

	/** Load game from a slot Id */
	bool LoadSlot(int32 SlotId, FOnGameLoaded OnLoaded = {});

	/** Load game from a SlotInfo */
	bool LoadSlot(const USlotInfo* SlotInfo, FOnGameLoaded OnLoaded = {});

	/** Reload the currently loaded slot if any */
	bool ReloadCurrentSlot(FOnGameLoaded OnLoaded = {})
	{
		return LoadSlot(CurrentInfo, MoveTemp(OnLoaded));
	}

	/**
	 * Find all saved games and return their SlotInfos
	 * @param bSortByRecent Should slots be ordered by save date?
	 * @param SaveInfos All saved games found on disk
	 */
	void LoadAllSlotInfos(bool bSortByRecent, FOnSlotInfosLoaded Delegate);
	void LoadAllSlotInfosSync(bool bSortByRecent, FOnSlotInfosLoaded Delegate);

	/** Delete a saved game on an specified slot name
	 * Performance: Interacts with disk, can be slow
	 */
	bool DeleteSlot(FName SlotName);

	/** Delete all saved slots from disk, loaded or not */
	void DeleteAllSlots(FOnSlotsDeleted Delegate);


	/** BLUEPRINT ONLY API */
public:
	// NOTE: This functions are mostly made to accommodate better Blueprint nodes that directly communicate
	// with the normal C++ API

	/** Save the Game into an specified Slot */
	UFUNCTION(Category = "SaveExtension|Saving", BlueprintCallable, meta = (AdvancedDisplay = "bScreenshot, Size",
		DisplayName = "Save Slot", Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPSaveSlot(FName SlotName, bool bScreenshot, const FScreenshotSize Size, ESaveGameResult& Result, FLatentActionInfo LatentInfo, bool bOverrideIfNeeded = true);

	/** Save the Game into an specified Slot */
	UFUNCTION(Category = "SaveExtension|Saving", BlueprintCallable, meta = (AdvancedDisplay = "bScreenshot, Size",
		DisplayName = "Save Slot by Id", Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPSaveSlotById(int32 SlotId, bool bScreenshot, const FScreenshotSize Size, ESaveGameResult& Result, FLatentActionInfo LatentInfo, bool bOverrideIfNeeded = true);

	/** Save the Game to a Slot */
	UFUNCTION(Category = "SaveExtension|Saving", BlueprintCallable, meta = (AdvancedDisplay = "bScreenshot, Size",
		DisplayName = "Save Slot by Info", Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPSaveSlotByInfo(const USlotInfo* SlotInfo, bool bScreenshot, const FScreenshotSize Size, ESaveGameResult& Result, FLatentActionInfo LatentInfo, bool bOverrideIfNeeded = true);

	/** Save the currently loaded Slot */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Saving",
		meta = (AdvancedDisplay = "bScreenshot, Size", DisplayName = "Save Current Slot", Latent,
			LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPSaveCurrentSlot(bool bScreenshot, const FScreenshotSize Size, ESaveGameResult& Result, FLatentActionInfo LatentInfo)
	{
		BPSaveSlotByInfo(CurrentInfo, bScreenshot, Size, Result, MoveTemp(LatentInfo), true);
	}

	/** Load game from a slot name */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading",
		meta = (DisplayName = "Load Slot", Latent, LatentInfo = "LatentInfo",
			ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPLoadSlot(FName SlotName, ELoadGameResult& Result, FLatentActionInfo LatentInfo);

	/** Load game from a slot Id */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading",
		meta = (DisplayName = "Load Slot by Id", Latent, LatentInfo = "LatentInfo",
			ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPLoadSlotById(int32 SlotId, ELoadGameResult& Result, FLatentActionInfo LatentInfo);

	/** Load game from a SlotInfo */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading",
		meta = (DisplayName = "Load Slot by Info", Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result",
			UnsafeDuringActorConstruction))
	void BPLoadSlotByInfo(const USlotInfo* SlotInfo, ELoadGameResult& Result, FLatentActionInfo LatentInfo);

	/** Reload the currently loaded slot if any */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading",
		meta = (DisplayName = "Reload Current Slot", Latent, LatentInfo = "LatentInfo",
			ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPReloadCurrentSlot(ELoadGameResult& Result, FLatentActionInfo LatentInfo)
	{
		BPLoadSlotByInfo(CurrentInfo, Result, MoveTemp(LatentInfo));
	}

	/**
	 * Find all saved games and return their SlotInfos
	 * @param bSortByRecent Should slots be ordered by save date?
	 * @param SaveInfos All saved games found on disk
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension",
		meta = (Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result",
			DisplayName = "Load All Slot Infos"))
	void BPLoadAllSlotInfos(const bool bSortByRecent, TArray<USlotInfo*>& SaveInfos, ELoadInfoResult& Result,
		struct FLatentActionInfo LatentInfo);

	/** Delete a saved game on an specified slot Id
	 * Performance: Interacts with disk, can be slow
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension")
	FORCEINLINE bool DeleteSlotById(int32 SlotId)
	{
		if (!IsValidSlot(SlotId))
		{
			return false;
		}
		return DeleteSlot(GetSlotNameFromId(SlotId));
	}

	/** Delete all saved slots from disk, loaded or not */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension",
		meta = (Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result",
			DisplayName = "Delete All Slots"))
	void BPDeleteAllSlots(EDeleteSlotsResult& Result, FLatentActionInfo LatentInfo);

	UFUNCTION(BlueprintPure, Category = "SaveExtension")
	USavePreset* BPGetPreset() const
	{
		return ActivePreset;
	}


	/** BLUEPRINTS & C++ API */
public:

	/** Delete a saved game on an specified slot
	 * Performance: Interacts with disk, can be slow
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension")
	bool DeleteSlot(USlotInfo* Slot)
	{
		return Slot? DeleteSlot(Slot->FileName) : false;
	}

	/** Get the currently loaded SlotInfo. If game was never loaded returns a new SlotInfo */
	UFUNCTION(BlueprintPure, Category = "SaveExtension")
	FORCEINLINE USlotInfo* GetCurrentInfo()
	{
		TryInstantiateInfo();
		return CurrentInfo;
	}

	/** Get the currently loaded SlotData. If game was never loaded returns an empty SlotData  */
	UFUNCTION(BlueprintPure, Category = "SaveExtension")
	FORCEINLINE USlotData* GetCurrentData()
	{
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
	FORCEINLINE USlotInfo* GetSlotInfoById(int32 SlotId)
	{
		return LoadInfo(SlotId);
	}

	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Slots")
	FORCEINLINE USlotInfo* GetSlotInfo(FName SlotName)
	{
		return LoadInfo(SlotName);
	}

	/** Check if an slot exists on disk
	 * @return true if the slot exists
	 */
	UFUNCTION(BlueprintPure, Category = "SaveExtension|Slots")
	bool IsSlotSaved(FName SlotName) const;

	/** Check if an slot exists on disk
	 * @return true if the slot exists
	 */
	UFUNCTION(BlueprintPure, Category = "SaveExtension|Slots")
	bool IsSlotSavedById(int32 SlotId) const
	{
		return IsValidSlot(SlotId)? IsSlotSaved(GetSlotNameFromId(SlotId)) : false;
	}

	/** Check if currently playing in a saved slot
	 * @return true if currently playing in a saved slot
	 */
	UFUNCTION(BlueprintPure, Category = "SaveExtension|Slots")
	FORCEINLINE bool IsInSlot() const
	{
		return CurrentInfo && CurrentData;
	}

	/**
	 * Set the preset to be used for saving and loading
	 * @return true if the preset was set successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension")
	USavePreset* SetActivePreset(TSubclassOf<USavePreset> PresetClass);

	const USavePreset* GetPreset() const;

	void TryInstantiateInfo(bool bForced = false);

	UFUNCTION(BlueprintPure, Category = "SaveExtension")
	FName GetSlotNameFromId(const int32 SlotId) const;

	bool IsValidSlot(const int32 Slot) const;

	void __SetCurrentInfo(USlotInfo* NewInfo)
	{
		CurrentInfo = NewInfo;
	}

	void __SetCurrentData(USlotData* NewData)
	{
		CurrentData = NewData;
	}

	USlotInfo* LoadInfo(FName SlotName);
	USlotInfo* LoadInfo(uint32 SlotId)
	{
		return IsValidSlot(SlotId)? LoadInfo(GetSlotNameFromId(SlotId)) : nullptr;
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

	void OnLevelLoaded(ULevelStreaming* StreamingLevel) {}

	USlotDataTask* CreateTask(TSubclassOf<USlotDataTask> TaskType);

	template <class TaskType>
	TaskType* CreateTask()
	{
		return Cast<TaskType>(CreateTask(TaskType::StaticClass()));
	}

	void FinishTask(USlotDataTask* Task);

public:
	bool HasTasks() const
	{
		return Tasks.Num() > 0;
	}

	/** @return true when saving or loading anything, including levels */
	UFUNCTION(BlueprintPure, Category = SaveExtension)
	bool IsSavingOrLoading() const
	{
		return HasTasks();
	}

	bool IsLoading() const;

protected:
	//~ Begin Tickable Object Interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual TStatId GetStatId() const override;
	//~ End Tickable Object Interface

	//~ Begin UObject Interface
	virtual UWorld* GetWorld() const override;
	//~ End UObject Interface


	/***********************************************************************/
	/* EVENTS                                                              */
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

	void OnSaveBegan(const FSELevelFilter& Filter);
	void OnSaveFinished(const FSELevelFilter& Filter, const bool bError);
	void OnLoadBegan(const FSELevelFilter& Filter);
	void OnLoadFinished(const FSELevelFilter& Filter, const bool bError);

private:
	void OnMapLoadStarted(const FString& MapName);
	void OnMapLoadFinished(UWorld* LoadedWorld);

	void IterateSubscribedInterfaces(TFunction<void(UObject*)>&& Callback);


	/***********************************************************************/
	/* STATIC                                                              */
	/***********************************************************************/
public:
	/** Get the global save manager */
	static USaveManager* Get(const UObject* ContextObject);


	/***********************************************************************/
	/* DEPRECATED                                                          */
	/***********************************************************************/

	UFUNCTION(Category = "SaveExtension|Saving", BlueprintCallable, meta = (DeprecatedFunction, DeprecationMessage="Use 'Save Slot by Id' instead.", 
		AdvancedDisplay = "bScreenshot, Size", DisplayName = "Save Slot to Id", Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPSaveSlotToId(int32 SlotId, bool bScreenshot, const FScreenshotSize Size, ESaveGameResult& Result, FLatentActionInfo LatentInfo, bool bOverrideIfNeeded = true)
	{
		BPSaveSlotById(SlotId, bScreenshot, Size, Result, LatentInfo, bOverrideIfNeeded);
	}

	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading", meta = (DeprecatedFunction, DeprecationMessage="Use 'Load Slot by Id' instead.",
		DisplayName = "Load Slot from Id", Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPLoadSlotFromId(int32 SlotId, ELoadGameResult& Result, FLatentActionInfo LatentInfo)
	{
		BPLoadSlotById(SlotId, Result, LatentInfo);
	}
};


inline bool USaveManager::SaveSlot(int32 SlotId, bool bOverrideIfNeeded, bool bScreenshot,
	const FScreenshotSize Size, FOnGameSaved OnSaved)
{
	if (!IsValidSlot(SlotId))
	{
		SELog(GetPreset(), "Invalid Slot. Cant go under 0 or exceed MaxSlots.", true);
		return false;
	}
	return SaveSlot(GetSlotNameFromId(SlotId), bOverrideIfNeeded, bScreenshot, Size, OnSaved);
}

inline bool USaveManager::SaveSlot(const USlotInfo* SlotInfo, bool bOverrideIfNeeded, bool bScreenshot,
	const FScreenshotSize Size, FOnGameSaved OnSaved)
{
	if (!SlotInfo)
	{
		return false;
	}
	return SaveSlot(SlotInfo->FileName, bOverrideIfNeeded, bScreenshot, Size, OnSaved);
}

inline void USaveManager::BPSaveSlotById(int32 SlotId, bool bScreenshot, const FScreenshotSize Size, ESaveGameResult& Result, FLatentActionInfo LatentInfo, bool bOverrideIfNeeded)
{
	if (!IsValidSlot(SlotId))
	{
		SELog(GetPreset(), "Invalid Slot. Cant go under 0 or exceed MaxSlots.", true);
		Result = ESaveGameResult::Failed;
		return;
	}
	BPSaveSlot(GetSlotNameFromId(SlotId), bScreenshot, Size, Result, MoveTemp(LatentInfo), bOverrideIfNeeded);
}

inline void USaveManager::BPSaveSlotByInfo(const USlotInfo* SlotInfo, bool bScreenshot, const FScreenshotSize Size,
	ESaveGameResult& Result, struct FLatentActionInfo LatentInfo, bool bOverrideIfNeeded)
{
	if (!SlotInfo)
	{
		Result = ESaveGameResult::Failed;
		return;
	}
	BPSaveSlot(SlotInfo->FileName, bScreenshot, Size, Result, MoveTemp(LatentInfo), bOverrideIfNeeded);
}

/** Save the currently loaded Slot */
inline bool USaveManager::SaveCurrentSlot(bool bScreenshot, const FScreenshotSize Size, FOnGameSaved OnSaved)
{
	return SaveSlot(CurrentInfo, true, bScreenshot, Size, OnSaved);
}

inline bool USaveManager::LoadSlot(int32 SlotId, FOnGameLoaded OnLoaded)
{
	if (!IsValidSlot(SlotId))
	{
		SELog(GetPreset(), "Invalid Slot. Can't go under 0 or exceed MaxSlots.", true);
		return false;
	}
	return LoadSlot(GetSlotNameFromId(SlotId), OnLoaded);
}

inline bool USaveManager::LoadSlot(const USlotInfo* SlotInfo, FOnGameLoaded OnLoaded)
{
	if (!SlotInfo)
	{
		return false;
	}
	return LoadSlot(SlotInfo->FileName, OnLoaded);
}

inline void USaveManager::BPLoadSlotById(
	int32 SlotId, ELoadGameResult& Result, struct FLatentActionInfo LatentInfo)
{
	BPLoadSlot(GetSlotNameFromId(SlotId), Result, MoveTemp(LatentInfo));
}

inline void USaveManager::BPLoadSlotByInfo(const USlotInfo* SlotInfo, ELoadGameResult& Result, FLatentActionInfo LatentInfo)
{
	if (!SlotInfo)
	{
		Result = ELoadGameResult::Failed;
		return;
	}
	BPLoadSlot(SlotInfo->FileName, Result, MoveTemp(LatentInfo));
}

inline bool USaveManager::IsValidSlot(const int32 Slot) const
{
	return GetPreset()->IsValidId(Slot);
}

inline void USaveManager::IterateSubscribedInterfaces(TFunction<void(UObject*)>&& Callback)
{
	for (const TScriptInterface<ISaveExtensionInterface>& Interface : SubscribedInterfaces)
	{
		if (UObject* const Object = Interface.GetObject())
		{
			Callback(Object);
		}
	}
}

inline USaveManager* USaveManager::Get(const UObject* Context)
{
	UWorld* World = GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::LogAndReturnNull);
	if (World)
	{
		return UGameInstance::GetSubsystem<USaveManager>(World->GetGameInstance());
	}
	return nullptr;
}

inline bool USaveManager::IsTickable() const
{
	return !HasAnyFlags(RF_ClassDefaultObject) && IsValid(this);
}

inline UWorld* USaveManager::GetTickableGameObjectWorld() const
{
	return bTickWithGameWorld? GetWorld() : nullptr;
}

inline TStatId USaveManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USaveManager, STATGROUP_Tickables);
}
