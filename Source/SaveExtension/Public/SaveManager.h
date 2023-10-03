// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "Delegates.h"
#include "LevelStreamingNotifier.h"
#include "SaveExtensionInterface.h"
#include "SaveSlot.h"
#include "SaveSlotData.h"
#include "Serialization/SEDataTask.h"

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
using FSEOnAllSlotsPreloaded = TFunction<void(const TArray<class USaveSlot*>& Slots)>;
using FSEOnAllSlotsDeleted = TFunction<void(int32 Count)>;


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

	friend FSEDataTask;


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

	UPROPERTY(Transient)
	TArray<ULevelStreamingNotifier*> LevelStreamingNotifiers;

	UPROPERTY(Transient)
	TArray<TScriptInterface<ISaveExtensionInterface>> SubscribedInterfaces;

	TArray<TUniquePtr<FSEDataTask>> Tasks;


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

	/** Save the currently loaded Slot */
	bool SaveActiveSlot(
		bool bScreenshot = false, const FScreenshotSize Size = {}, FOnGameSaved OnSaved = {});


	/** Load game from a file name */
	bool LoadSlot(FName SlotName, FOnGameLoaded OnLoaded = {});

	/** Load game from a Slot */
	bool LoadSlot(const USaveSlot* Slot, FOnGameLoaded OnLoaded = {});

	/** Reload the currently loaded slot if any */
	bool ReloadActiveSlot(FOnGameLoaded OnLoaded = {})
	{
		return LoadSlot(ActiveSlot, MoveTemp(OnLoaded));
	}

	/**
	 * Find all saved slots and preload them, without loading their data
	 * @param Slots preloaded from on disk
	 * @param bSortByRecent Should slots be ordered by save date?
	 */
	void PreloadAllSlots(FSEOnAllSlotsPreloaded Callback, bool bSortByRecent = false);
	/**
	 * Find all saved slots and preload them asynchronously, without loading their data
	 * Performance: Interacts with disk, can be slow
	 * @param Slots preloaded from on disk
	 * @param bSortByRecent Should slots be ordered by save date?
	 */
	void PreloadAllSlotsSync(TArray<USaveSlot*>& Slots, bool bSortByRecent = false);

	/** Delete a saved game on an specified slot name
	 * Performance: Interacts with disk, can be slow
	 */
	bool DeleteSlotByNameSync(FName SlotName);
	/** Deletes all saved slots in disk. Does not affect slots in memory.
	 * Performance: Interacts with disk, can be slow
	 */
	int32 DeleteAllSlotsSync();
	/** Deletes all saved slots in disk. Does not affect slots in memory. */
	void DeleteAllSlots(FSEOnAllSlotsDeleted Delegate);


	/** BLUEPRINT ONLY API */
public:
	// NOTE: This functions are mostly made to accommodate better Blueprint nodes that directly communicate
	// with the normal C++ API

	/** Save the Game into an specified Slot */
	UFUNCTION(Category = "SaveExtension|Saving", BlueprintCallable,
		meta = (AdvancedDisplay = "bScreenshot, Size", DisplayName = "Save Slot by Name", Latent,
			LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPSaveSlotByName(FName SlotName, bool bScreenshot,
		UPARAM(meta = (EditCondition = bScreenshot)) const FScreenshotSize Size, ESEContinueOrFail& Result,
		FLatentActionInfo LatentInfo, bool bOverrideIfNeeded = true);

	/** Save the Game to a Slot */
	UFUNCTION(Category = "SaveExtension|Saving", BlueprintCallable,
		meta = (AdvancedDisplay = "bScreenshot, Size", DisplayName = "Save Slot", Latent,
			LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPSaveSlot(const USaveSlot* Slot, bool bScreenshot,
		UPARAM(meta = (EditCondition = bScreenshot)) const FScreenshotSize Size, ESEContinueOrFail& Result,
		FLatentActionInfo LatentInfo, bool bOverrideIfNeeded = true);

	/** Save the currently loaded Slot */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Saving",
		meta = (AdvancedDisplay = "bScreenshot, Size", DisplayName = "Save Active Slot", Latent,
			LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPSaveActiveSlot(bool bScreenshot,
		UPARAM(meta = (EditCondition = bScreenshot)) const FScreenshotSize Size, ESEContinueOrFail& Result,
		FLatentActionInfo LatentInfo)
	{
		BPSaveSlot(ActiveSlot, bScreenshot, Size, Result, MoveTemp(LatentInfo), true);
	}

	/** Load game from a slot name */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading",
		meta = (DisplayName = "Load Slot by Name", Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result",
			UnsafeDuringActorConstruction))
	void BPLoadSlotByName(FName SlotName, ESEContinueOrFail& Result, FLatentActionInfo LatentInfo);

	/** Load game from a Slot */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading",
		meta = (DisplayName = "Load Slot", Latent, LatentInfo = "LatentInfo",
			ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPLoadSlot(const USaveSlot* Slot, ESEContinueOrFail& Result, FLatentActionInfo LatentInfo);

	/** Reload the currently loaded slot if any */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading",
		meta = (DisplayName = "Reload Active Slot", Latent, LatentInfo = "LatentInfo",
			ExpandEnumAsExecs = "Result", UnsafeDuringActorConstruction))
	void BPReloadActiveSlot(ESEContinueOrFail& Result, FLatentActionInfo LatentInfo)
	{
		BPLoadSlot(ActiveSlot, Result, MoveTemp(LatentInfo));
	}

	/**
	 * Find all saved slots and preload them, without loading their data
	 * @param Slots preloaded from on disk
	 * @param bSortByRecent Should slots be ordered by save date?
	 */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension",
		meta = (Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result",
			DisplayName = "Preload All Slots"))
	void BPPreloadAllSlots(const bool bSortByRecent, TArray<USaveSlot*>& Slots, ESEContinue& Result,
		struct FLatentActionInfo LatentInfo);

	/** Delete all saved slots from disk, loaded or not */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension",
		meta = (Latent, LatentInfo = "LatentInfo", ExpandEnumAsExecs = "Result",
			DisplayName = "Delete All Slots"))
	void BPDeleteAllSlots(ESEContinue& Result, FLatentActionInfo LatentInfo);


	/** BLUEPRINTS & C++ API */
public:
	/** Delete a saved game on an specified slot name */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension")
	void DeleteSlotByName(FName SlotName);

	/** Delete a saved game on an specified slot */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension")
	void DeleteSlot(USaveSlot* Slot)
	{
		if (Slot)
		{
			DeleteSlotByName(Slot->FileName);
		}
	}

	/** Get the currently loaded Slot. If game was never loaded returns a new Slot */
	UFUNCTION(BlueprintPure, Category = "SaveExtension")
	FORCEINLINE USaveSlot* GetActiveSlot()
	{
		AssureActiveSlot();
		return ActiveSlot;
	}

	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Slots")
	USaveSlot* PreloadSlot(FName SlotName);

	/** Check if an slot exists on disk
	 * @return true if the slot exists
	 */
	UFUNCTION(BlueprintPure, Category = "SaveExtension|Slots")
	bool IsSlotSaved(FName SlotName) const;


	/** Check if currently playing in a saved slot
	 * @return true if currently playing in a saved slot
	 */
	UFUNCTION(BlueprintPure, Category = "SaveExtension|Slots")
	FORCEINLINE bool HasActiveSlot() const
	{
		return ActiveSlot != nullptr;
	}

	// Assigns a new active slot. If this slot is preloaded, empty data is assigned to it.
	// This does not load the game!
	void SetActiveSlot(USaveSlot* NewSlot);

	void AssureActiveSlot(TSubclassOf<USaveSlot> ActiveSlotClass = {}, bool bForced = false);

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

	template <typename TaskType>
	TaskType& CreateTask()
	{
		return static_cast<TaskType&>(
			*Tasks.Add_GetRef(MakeUnique<TaskType>(this, ActiveSlot))
		);
	}

	void FinishTask(FSEDataTask* Task);

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

	void OnSaveBegan();
	void OnSaveFinished(const bool bError);
	void OnLoadBegan();
	void OnLoadFinished(const bool bError);

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
};


inline bool USaveManager::SaveSlot(const USaveSlot* Slot, bool bOverrideIfNeeded, bool bScreenshot,
	const FScreenshotSize Size, FOnGameSaved OnSaved)
{
	if (!Slot)
	{
		return false;
	}
	return SaveSlot(Slot->FileName, bOverrideIfNeeded, bScreenshot, Size, OnSaved);
}

inline void USaveManager::BPSaveSlot(const USaveSlot* Slot, bool bScreenshot,
	const FScreenshotSize Size, ESEContinueOrFail& Result, struct FLatentActionInfo LatentInfo,
	bool bOverrideIfNeeded)
{
	if (!Slot)
	{
		Result = ESEContinueOrFail::Failed;
		return;
	}
	BPSaveSlotByName(Slot->FileName, bScreenshot, Size, Result, MoveTemp(LatentInfo), bOverrideIfNeeded);
}

inline bool USaveManager::SaveActiveSlot(bool bScreenshot, const FScreenshotSize Size, FOnGameSaved OnSaved)
{
	return SaveSlot(ActiveSlot, true, bScreenshot, Size, OnSaved);
}

inline bool USaveManager::LoadSlot(const USaveSlot* Slot, FOnGameLoaded OnLoaded)
{
	if (!Slot)
	{
		return false;
	}
	return LoadSlot(Slot->FileName, OnLoaded);
}

inline void USaveManager::BPLoadSlot(
	const USaveSlot* Slot, ESEContinueOrFail& Result, FLatentActionInfo LatentInfo)
{
	if (!Slot)
	{
		Result = ESEContinueOrFail::Failed;
		return;
	}
	BPLoadSlotByName(Slot->FileName, Result, MoveTemp(LatentInfo));
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
