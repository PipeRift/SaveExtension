// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "Engine/DataAsset.h"
#include "Settings.h"
#include "SaveGraph.generated.h"

UENUM(BlueprintType)
enum class ESaveActorFilterMode : uint8 {
	RootLevel,
	SubLevels,
	AllLevels
};


/**
 * What to save, how to save it, when, every x minutes, what info file, what data file, save by level streaming?
 */
UCLASS(Blueprintable, ClassGroup = SaveExtension, Config = Game)
class SAVEEXTENSION_API USaveGraph : public UObject
{
	GENERATED_BODY()

public:

	USaveGraph() : Super() {}

	bool DoPrepare() { return EventPrepare(); }

protected:

	virtual bool Prepare() { return true; }

	/**
	 * Prepares Packets for serialization, specifying what and in which conditions to save
	 * @returns true to continue saving and false to fail
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "SaveExtension|Graph", meta = (DisplayName = "Prepare"))
	bool EventPrepare();


	/************************************************************************/
	/* Packets                                                              */
	/************************************************************************/

	/** Serializes all actors allowed by the filter */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Graph|Packets")
	void AddActorPacket(const FActorClassFilter& Filter/*, const FActorPacketSettings& Settings, TSubclassOf<UActorSerializer> CustomSerializer*/) {}

	/** Serializes all components allowed by the filter */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Graph|Packets")
	void AddComponentPacket(const FComponentClassFilter& Filter/*, const FComponentPacketSettings& Settings, TSubclassOf<UComponentSerializer> CustomSerializer*/) {}

	/** Serializes all components allowed by the filter */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Graph|Packets")
	void AddObjectPacket(const FClassFilter& Filter/*, const FComponentPacketSettings& Settings, TSubclassOf<UComponentSerializer> CustomSerializer*/) {}


	/************************************************************************/
	/* Levels                                                               */
	/************************************************************************/

	/** Serializes root level */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Graph|Levels")
	void SerializeRootLevel() {}

	/** Serializes a sub-level if loaded */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Graph|Levels")
	void SerializeSubLevel(const FName& Level) { SerializeSubLevels({Level}); }

	/** Serializes a list of currently loaded sub-levels on the world */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Graph|Levels")
	void SerializeSubLevels(const TArray<FName>& Levels) {}

	/** Serializes all currently loaded sub-levels on the world */
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Graph|Levels")
	void SerializeAllSubLevels() {}


	/************************************************************************/
	/* Serialized File/Data storing                                         */
	/************************************************************************/

	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Graph|Storing")
	void SaveToFile(const FString& FileName) {}
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Graph|Storing")
	void KeepInMemory() {}
	UFUNCTION(BlueprintCallable, Category = "SaveExtension|Graph|Storing")
	virtual void CustomSave() {}


public:

	virtual UWorld* GetWorld() const override;
	class USaveManager* GetManager() const;
};
