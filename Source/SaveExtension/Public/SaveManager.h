// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "Delegates.h"
#include "LevelStreamingNotifier.h"
#include "Multithreading/Delegates.h"
#include "Multithreading/ScopedTaskManager.h"
#include "SaveExtensionInterface.h"
#include "SaveSlot.h"
#include "SaveSlotData.h"
#include "Serialization/SlotDataTask.h"

#include <Async/AsyncWork.h>
#include <CoreMinimal.h>
#include <Engine/GameInstance.h>
#include <GenericPlatform/GenericPlatformFile.h>
#include <HAL/PlatformFilemanager.h>
#include <Subsystems/GameInstanceSubsystem.h>
#include <Tickable.h>

#include "SaveManager.generated.h"


struct FLatentActionInfo;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameSavedMC, USaveSlot*, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameLoadedMC, USaveSlot*, Slot);


UENUM()
enum class ESEContinue : uint8
{
	InProgress UMETA(Hidden),
	Continue
};

UENUM()
enum class ESEContinueOrFail : uint8
{
	InProgress UMETA(Hidden),
	Continue,
	Failed
};


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

	friend USaveSlotDataTask;


	/************************************************************************/
	/* PROPERTIES														    */
	/************************************************************************/
public:
	// Loaded from settings. Can be changed at runtime
	UPROPERTY(Transient, BlueprintReadWrite, Category = SaveManager)
	bool bTickWithGameWorld = false;

private:
	// Active SaveSlot used for current saves (load on start, periodic save, etc)
	UPROPERTY()
	TObjectPtr<USaveSlot> ActiveSlot;

	/** The game instance to which this save manager is owned. */
	TWeakObjectPtr<UGameInstance> OwningGameInstance;

	FScopedTaskList MTTasks;

	UPROPERTY(Transient)
	TArray<ULevelStreamingNotifier*> LevelStreamingNotifiers;

	UPROPERTY(Transient)
	TArray<TScriptInterface<ISaveExtensionInterface>> SubscribedInterfaces;

	UPROPERTY(Transient)
	TArray<USaveSlotDataTask*> Tasks;


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

	/** Save the Game info an Slot */
	bool SaveSlot(const USaveSlot* Slot, bool bOverrideIfNeeded = true, bool bScreenshot = false,
		const FScreenshotSize Size = {}, FOnGameSaved OnSaved = {});

	/** Save the Game into an specified slot id */
	bool SaveSlot(int32 SlotId, bool bOverrideIfNeeded = true, bool bScreenshot = false,
		const FScreenshotSize Size = {}, FOnGameSaved OnSaved = {});

	/** Save the currently loaded Slot */
	bool SaveCurrentSlot(
		bool bScreenshot = false, const FScreenshotSize Size = {}, FOnGameSaved OnSaved = {});


	/** Load game from a file name */
	bool LoadSlot(FName SlotName, FOnGameLoaded OnLoaded = {});

	/** Load game from a slot Id */
	bool LoadSlot(int32 SlotId, FOnGameLoaded OnLoaded = {});

	/** Load game from a Slot */
	bool LoadSlot(const USaveSlot* Slot, FOnGameLoaded OnLoaded = {});

	/** Reload the currently loaded slot if any */
	bool ReloadCurrentSlot(FOnGameLoaded OnLoaded = {})
	{
		return LoadSlot(ActiveSlot, MoveTemp(OnLoaded));
	}

	/**
	 * Find all saved games and return their Slots
	 * @param bSortByRecent Should slots be ordered by save date?
	 * @param SaveInfos All saved games found on disk
	 */
	void FindAllSlots(bool bSortByRecent, FOnSlotsLoaded Delegate);
	void FindAllSlotsSync(bool bSortByRecent, TArray<USaveSlot*>& Slots);

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
	UFUNCTION(Category = "SaveExtension|Saving", BlueprintCallable,
		meta = (AdvancedDisplay = "bScreenshot, Size", DisplayName = "Save Slot", Latent,
			LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPSaveSlot(FName SlotName, bool bScreenshot,
		UPARAM(meta = (EditCondition = bScreenshot)) const FScreenshotSize Size, ESEContinueOrFail& Result,
		FLatentActionInfo LatentInfo, bool bOverrideIfNeeded = true);

	/** Save the Game into an specified Slot */
	UFUNCTION(Category = "SaveExtension|Saving", BlueprintCallable,
		meta = (AdvancedDisplay = "bScreenshot, Size", DisplayName = "Save Slot by Id", Latent,
			LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPSaveSlotById(int32 SlotId, bool bScreenshot,
		UPARAM(meta = (EditCondition = bScreenshot)) const FScreenshotSize Size, ESEContinueOrFail& Result,
		FLatentActionInfo LatentInfo, bool bOverrideIfNeeded = true);

	/** Save the Game to a Slot */
	UFUNCTION(Category = "SaveExtension|Saving", BlueprintCallable,
		meta = (AdvancedDisplay = "bScreenshot, Size", DisplayName = "Save Slot by Info", Latent,
			LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPSaveSlotByInfo(const USaveSlot* Slot, bool bScreenshot,
		UPARAM(meta = (EditCondition = bScreenshot)) const FScreenshotSize Size, ESEContinueOrFail& Result,
		FLatentActionInfo LatentInfo, bool bOverrideIfNeeded = true);

	/** Save the currently loaded Slot */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Saving",
		meta = (AdvancedDisplay = "bScreenshot, Size", DisplayName = "Save Current Slot", Latent,
			LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPSaveCurrentSlot(bool bScreenshot,
		UPARAM(meta = (EditCondition = bScreenshot)) const FScreenshotSize Size, ESEContinueOrFail& Result,
		FLatentActionInfo LatentInfo)
	{
		BPSaveSlotByInfo(ActiveSlot, bScreenshot, Size, Result, MoveTemp(LatentInfo), true);
	}

	/** Load game from a slot name */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading",
		meta = (DisplayName = "Load Slot", Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result",
			UnsafeDuringActorConstruction))
	void BPLoadSlot(FName SlotName, ESEContinueOrFail& Result, FLatentActionInfo LatentInfo);

	/** Load game from a slot Id */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading",
		meta = (DisplayName = "Load Slot by Id", Latent, LatentInfo = "LatentInfo",
			ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPLoadSlotById(int32 SlotId, ESEContinueOrFail& Result, FLatentActionInfo LatentInfo);

	/** Load game from a Slot */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading",
		meta = (DisplayName = "Load Slot by Info", Latent, LatentInfo = "LatentInfo",
			ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPLoadSlotByInfo(const USaveSlot* Slot, ESEContinueOrFail& Result, FLatentActionInfo LatentInfo);

	/** Reload the currently loaded slot if any */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading",
		meta = (DisplayName = "Reload Current Slot", Latent, LatentInfo = "LatentInfo",
			ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPReloadCurrentSlot(ESEContinueOrFail& Result, FLatentActionInfo LatentInfo)
	{
		BPLoadSlotByInfo(ActiveSlot, Result, MoveTemp(LatentInfo));
	}

	/**
	 * Find all saved games and return their Slots
	 * @param bSortByRecent Should slots be ordered by save date?
	 * @param SaveInfos All saved games found on disk
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension",
		meta = (Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result",
			DisplayName = "Find All Slots"))
	void BPFindAllSlots(const bool bSortByRecent, TArray<USaveSlot*>& Slots, ESEContinue& Result,
		struct FLatentActionInfo LatentInfo);

	/** Delete a saved game on an specified slot Id
	 * Performance: Interacts with disk, can be slow
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension")
	FORCEINLINE bool DeleteSlotById(int32 SlotId)
	{
		return DeleteSlot(GetFileNameFromId(SlotId));
	}

	/** Delete all saved slots from disk, loaded or not */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension",
		meta = (Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result",
			DisplayName = "Delete All Slots"))
	void BPDeleteAllSlots(ESEContinue& Result, FLatentActionInfo LatentInfo);


	/** BLUEPRINTS & C++ API */
public:
	/** Delete a saved game on an specified slot
	 * Performance: Interacts with disk, can be slow
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension")
	bool DeleteSlot(USaveSlot* Slot)
	{
		return Slot ? DeleteSlot(Slot->FileName) : false;
	}

	/** Get the currently loaded Slot. If game was never loaded returns a new Slot */
	UFUNCTION(BlueprintPure, Category = "SaveExtension")
	FORCEINLINE USaveSlot* GetActiveSlot()
	{
		AssureActiveSlot();
		return ActiveSlot;
	}

	/**
	 * Load and return an Slot by Id if it exists
	 * Performance: Interacts with disk, could be slow if called frequently
	 * @param SlotId Id of the Slot to be loaded
	 * @return the Slot associated with an Id
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Slots")
	FORCEINLINE USaveSlot* GetSlotById(int32 SlotId)
	{
		return LoadInfo(SlotId);
	}

	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Slots")
	FORCEINLINE USaveSlot* GetSlot(FName SlotName)
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
		if (ActiveSlot && ActiveSlot->IsValidIndex(SlotId))
		{
			return IsSlotSaved(GetFileNameFromId(SlotId));
		}
		return false;
	}

	/** Check if currently playing in a saved slot
	 * @return true if currently playing in a saved slot
	 */
	UFUNCTION(BlueprintPure, Category = "SaveExtension|Slots")
	FORCEINLINE bool IsInSlot() const
	{
		return ActiveSlot != nullptr;
	}

	void AssureActiveSlot(TSubclassOf<USaveSlot> ActiveSlotClass = {}, bool bForced = false);

	UFUNCTION(BlueprintPure, Category = "SaveExtension")
	FName GetFileNameFromId(const int32 SlotId) const;


	void AssignActiveSlot(USaveSlot* NewInfo)
	{
		ActiveSlot = NewInfo;
	}

	USaveSlot* LoadInfo(FName FileName);
	USaveSlot* LoadInfo(uint32 SlotId)
	{
		return LoadInfo(GetFileNameFromId(SlotId));
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

	USaveSlotDataTask* CreateTask(TSubclassOf<USaveSlotDataTask> TaskType);

	template <class TaskType>
	TaskType* CreateTask()
	{
		return Cast<TaskType>(CreateTask(TaskType::StaticClass()));
	}

	void FinishTask(USaveSlotDataTask* Task);

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

	UFUNCTION(Category = "SaveExtension|Saving", BlueprintCallable,
		meta = (DeprecatedFunction, DeprecationMessage = "Use 'Save Slot by Id' instead.",
			AdvancedDisplay = "bScreenshot, Size", DisplayName = "Save Slot to Id", Latent,
			LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPSaveSlotToId(int32 SlotId, bool bScreenshot, const FScreenshotSize Size, ESEContinueOrFail& Result,
		FLatentActionInfo LatentInfo, bool bOverrideIfNeeded = true)
	{
		BPSaveSlotById(SlotId, bScreenshot, Size, Result, LatentInfo, bOverrideIfNeeded);
	}

	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading",
		meta = (DeprecatedFunction, DeprecationMessage = "Use 'Load Slot by Id' instead.",
			DisplayName = "Load Slot from Id", Latent, LatentInfo = "LatentInfo",
			ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPLoadSlotFromId(int32 SlotId, ESEContinueOrFail& Result, FLatentActionInfo LatentInfo)
	{
		BPLoadSlotById(SlotId, Result, LatentInfo);
	}
};


inline bool USaveManager::SaveSlot(
	int32 SlotId, bool bOverrideIfNeeded, bool bScreenshot, const FScreenshotSize Size, FOnGameSaved OnSaved)
{
	if (!ActiveSlot->IsValidIndex(SlotId))
	{
		UE_LOG(LogSaveExtension, Error, TEXT("Can't save to slot id under 0 or exceeding MaxSlots."));
		return false;
	}
	return SaveSlot(GetFileNameFromId(SlotId), bOverrideIfNeeded, bScreenshot, Size, OnSaved);
}

inline bool USaveManager::SaveSlot(const USaveSlot* Slot, bool bOverrideIfNeeded, bool bScreenshot,
	const FScreenshotSize Size, FOnGameSaved OnSaved)
{
	if (!Slot)
	{
		return false;
	}
	return SaveSlot(Slot->FileName, bOverrideIfNeeded, bScreenshot, Size, OnSaved);
}

inline void USaveManager::BPSaveSlotById(int32 SlotId, bool bScreenshot, const FScreenshotSize Size,
	ESEContinueOrFail& Result, FLatentActionInfo LatentInfo, bool bOverrideIfNeeded)
{
	if (!ActiveSlot || !ActiveSlot->IsValidIndex(SlotId))
	{
		UE_LOG(LogSaveExtension, Error, TEXT("Invalid Slot. Cant go under 0 or exceed MaxSlots."));
		Result = ESEContinueOrFail::Failed;
		return;
	}
	BPSaveSlot(GetFileNameFromId(SlotId), bScreenshot, Size, Result, MoveTemp(LatentInfo), bOverrideIfNeeded);
}

inline void USaveManager::BPSaveSlotByInfo(const USaveSlot* Slot, bool bScreenshot,
	const FScreenshotSize Size, ESEContinueOrFail& Result, struct FLatentActionInfo LatentInfo,
	bool bOverrideIfNeeded)
{
	if (!Slot)
	{
		Result = ESEContinueOrFail::Failed;
		return;
	}
	BPSaveSlot(Slot->FileName, bScreenshot, Size, Result, MoveTemp(LatentInfo), bOverrideIfNeeded);
}

/** Save the currently loaded Slot */
inline bool USaveManager::SaveCurrentSlot(bool bScreenshot, const FScreenshotSize Size, FOnGameSaved OnSaved)
{
	return SaveSlot(ActiveSlot, true, bScreenshot, Size, OnSaved);
}

inline bool USaveManager::LoadSlot(int32 SlotId, FOnGameLoaded OnLoaded)
{
	if (!ActiveSlot->IsValidIndex(SlotId))
	{
		UE_LOG(LogSaveExtension, Error, TEXT("Invalid Slot. Can't go under 0 or exceed MaxSlots."));
		return false;
	}
	return LoadSlot(GetFileNameFromId(SlotId), OnLoaded);
}

inline bool USaveManager::LoadSlot(const USaveSlot* Slot, FOnGameLoaded OnLoaded)
{
	if (!Slot)
	{
		return false;
	}
	return LoadSlot(Slot->FileName, OnLoaded);
}

inline void USaveManager::BPLoadSlotById(
	int32 SlotId, ESEContinueOrFail& Result, struct FLatentActionInfo LatentInfo)
{
	BPLoadSlot(GetFileNameFromId(SlotId), Result, MoveTemp(LatentInfo));
}

inline void USaveManager::BPLoadSlotByInfo(
	const USaveSlot* Slot, ESEContinueOrFail& Result, FLatentActionInfo LatentInfo)
{
	if (!Slot)
	{
		Result = ESEContinueOrFail::Failed;
		return;
	}
	BPLoadSlot(Slot->FileName, Result, MoveTemp(LatentInfo));
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
	return bTickWithGameWorld ? GetWorld() : nullptr;
}

inline TStatId USaveManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USaveManager, STATGROUP_Tickables);
}
