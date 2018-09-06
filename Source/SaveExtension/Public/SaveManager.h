// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <PlatformFilemanager.h>
#include <GenericPlatformFile.h>
#include <Engine/GameInstance.h>
#include <Queue.h>

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

	/** Currently active SaveInfo. SaveInfo stores basic information about a saved game. Played time, levels, progress, etc. */
	UPROPERTY()
	USlotInfo* CurrentInfo;

	/** Currently active SaveData. SaveData stores all serialized info about the world. */
	UPROPERTY()
	USlotData* CurrentData;

	/** The game instance to which this save manager is owned. */
	TWeakObjectPtr<UGameInstance> OwningGameInstance;

	UPROPERTY(Transient)
	TArray<ULevelStreamingNotifier*> LevelStreamingNotifiers;


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
	bool SaveGameToSlot(int32 Slot = 0, bool bOverrideIfNeeded = true, bool bScreenshot = false, const int32 Width = 640, const int32 Height = 360);

	/** Save the Game to a Slot */
	UFUNCTION(Category = "SaveExtension|Saving", BlueprintCallable, meta = (AdvancedDisplay = 2))
	FORCEINLINE bool SaveGameFromInfo(const USlotInfo* SlotInfo, bool bOverrideIfNeeded = true, bool bScreenshot = false, const int32 Width = 640, const int32 Height = 360) {
		if (!SlotInfo) return false;
		return SaveGameToSlot(SlotInfo->Id, bOverrideIfNeeded, bScreenshot, Width, Height);
	}

	/** Save the currently loaded Slot */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Saving", meta = (AdvancedDisplay = 1))
	bool SaveCurrentSlot(bool bScreenshot = false, const int32 Width = 640, const int32 Height = 360) {
		return CurrentInfo ? SaveGameToSlot(CurrentInfo->Id, true, bScreenshot, Width, Height) : false;
	}

	/** Load an specified Slot */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading")
	bool LoadGame(int32 Slot = 0);

	/** Load the Game from a Slot. */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading")
	FORCEINLINE bool LoadGameFromInfo(const USlotInfo* SlotInfo) {
		return SlotInfo ? LoadGame(SlotInfo->Id) : false;
	}

	/** Reload the currently loaded slot */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading")
	bool ReloadCurrentSlot() { return CurrentInfo ? LoadGame(CurrentInfo->Id) : false; }

	/** Delete a saved game on an specified slot */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading")
	bool DeleteGame(int32 Slot);

	/** Returns given slot's loaded thumbnail */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Loading", meta = (DisplayName = "Load Game Screenshot"))
	UTexture2D* LoadThumbnail(int32 Slot);

	/** Check if an slot is saved on disk*/
	UFUNCTION(Category = "SaveExtension|Other", BlueprintPure)
	bool IsSlotSaved(int32 Slot) const;

	/** @return the current SlotInfo. Will usually come from the last loaded slot */
	UFUNCTION(Category = "SaveExtension|Other", BlueprintPure)
	FORCEINLINE USlotInfo* GetCurrentInfo() {
		TryInstantiateInfo();
		return CurrentInfo;
	}

	/** @return the current SlotData. Will usually come from the last loaded slot */
	UFUNCTION(Category = "SaveExtension|Other", BlueprintPure)
	FORCEINLINE USlotData* GetCurrentData() {
		TryInstantiateInfo();
		return CurrentData;
	}

	/** @return the SlotInfo associated with an slot */
	UFUNCTION(Category = "SaveExtension|Loading", BlueprintCallable)
	FORCEINLINE USlotInfo* GetInfoFromSlot(int32 SlotId) {
		return LoadInfo(SlotId);
	}

	UFUNCTION(Category = "SaveExtension|Loading", BlueprintCallable)
	void GetAllSlotInfos(TArray<USlotInfo*>& SaveInfos, const bool SortByRecent = false);

	/** @return true if playing in an active slot */
	UFUNCTION(Category = "SaveExtension", BlueprintPure)
	FORCEINLINE bool IsInSlot() const { return CurrentInfo && CurrentData; }

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

	/** Saves a thumbnail for the current slot */
	bool SaveThumbnail(const int32 Slot, const int32 Width = 640, const int32 Height = 360);

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


	UFUNCTION(Category = SaveExtension, BlueprintCallable)
	void RegistrySaveInterface(const TScriptInterface<ISaveExtensionInterface>& Interface);

	UFUNCTION(Category = SaveExtension, BlueprintCallable)
	void UnregistrySaveInterface(const TScriptInterface<ISaveExtensionInterface>& Interface);

private:

	UPROPERTY(Transient)
	TArray<TScriptInterface<ISaveExtensionInterface>> RegisteredInterfaces;

	void OnSaveBegan();
	void OnSaveFinished(const bool bError);
	void OnLoadBegan();
	void OnLoadFinished(const bool bError);

	void OnMapLoadStarted(const FString& MapName);
	void OnMapLoadFinished(UWorld* LoadedWorld);


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

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory);
	};

	static TMap<TWeakObjectPtr<UGameInstance>, TWeakObjectPtr<USaveManager>> GlobalManagers;

public:

	UFUNCTION(BlueprintPure, Category = SaveExtension, meta = (WorldContext = "ContextObject"))
	static USaveManager* GetSaveManager(const UObject* ContextObject);
};
