// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "Engine/DataAsset.h"
#include "SavePreset.generated.h"

class USlotInfo;
class USlotData;


/**
 * What to save, how to save it, when, every x minutes, what info file, what data file, save by chunks?
 */
UCLASS(ClassGroup = SaveExtension, BlueprintType, Config = Game)
class SAVEEXTENSION_API USavePreset : public UDataAsset
{
	GENERATED_BODY()

public:

	USavePreset();


public:

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
	 * Ignored in package on Shipping mode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, Config, AdvancedDisplay)
	bool bDebug;

	/**
	 * If checked and Debug is enabled, will print messages to Viewport.
	 * Ignored in package on Shipping mode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay, Config, AdvancedDisplay, meta = (EditCondition = "bDebug"))
	bool bDebugInScreen;


	/* Actors referenced in this list will be explicitly ignored by the Save/Load system. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Serialization, AdvancedDisplay)
	TArray<TSubclassOf<AActor>> NonSerializedActors;

	/** True if Gamemode should be saved  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Serialization, Config)
	bool bStoreGameMode;

	/** Should save and load GameInstance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Serialization, Config)
	bool bStoreGameInstance;

	/** Should save and load all the Level Blueprints on the world */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Serialization, Config)
	bool bStoreLevelBlueprints;

	/** Should save and load 'SaveGame' variables for AI Controllers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Serialization, Config)
	bool bStoreAIControllers;

	/** Should save and load all Blueprint's Actor Components.
	Note: Static Mesh Components and Skeletal Components are not serialized. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Serialization, Config)
	bool bStoreComponents;

	/** Should save and load all Blueprint's Actor Components.
	Note: Static Mesh Components and Skeletal Components are not serialized. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization|Controllers", Config)
	bool bStoreControlRotation;

	FORCEINLINE int32 GetMaxSlots() const {
		return MaxSlots <= 0 ? 16384 : MaxSlots;
	}
};
