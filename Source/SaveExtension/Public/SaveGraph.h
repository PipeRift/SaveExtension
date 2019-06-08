// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "Engine/DataAsset.h"
#include "PipelineSettings.h"
#include "SaveGraph.generated.h"

UENUM(BlueprintType)
enum class ESaveActorFilterMode : uint8 {
	RootLevel,
	SubLevels,
	AllLevels
};

USTRUCT(BlueprintType)
struct FSaveActorFilter
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Filter)
	ESaveActorFilterMode LevelMode = ESaveActorFilterMode::AllLevels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Filter)
	TArray<FName> LevelNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Filter)
	FSettingsActorFilter ClassFilter;
};



UCLASS()
class UPipelineTask : public UObject
{
	GENERATED_BODY()

};


/**
 * What to save, how to save it, when, every x minutes, what info file, what data file, save by level streaming?
 */
UCLASS(Blueprintable, ClassGroup = SaveExtension, Config = Game)
class SAVEEXTENSION_API USaveGraph : public UObject
{
	GENERATED_BODY()

private:

	UPROPERTY(EditAnywhere, Category = "Pipeline")
	FPipelineSettings Settings;

	UPROPERTY(Transient)
	TArray<UPipelineTask*> TaskQueue;

public:

	USaveGraph() : Super() {}

	void DoBeginPlay()           { EventBeginPlay(); }
	void DoTick(float DeltaTime) { EventTick(DeltaTime); }
	void DoSerializeSlot()       { EventSerializeSlot(); }
	void DoEndPlay()             { EventEndPlay(); }

protected:

	virtual void BeginPlay();
	UFUNCTION(BlueprintNativeEvent, Category = "Pipeline", meta = (DisplayName = "Begin Play"))
	void EventBeginPlay();

	virtual void Tick(float DeltaTime) {}
	UFUNCTION(BlueprintNativeEvent, Category = "Pipeline", meta=(DisplayName = "Tick"))
	void EventTick(float DeltaTime);

	virtual void SerializeSlot() {}
	UFUNCTION(BlueprintNativeEvent, Category = "Pipeline", meta = (DisplayName = "Serialize Slot"))
	void EventSerializeSlot();

	virtual void EndPlay();
	UFUNCTION(BlueprintNativeEvent, Category = "Pipeline", meta = (DisplayName = "End Play"))
	void EventEndPlay();


	/** Serializes root level */
	UFUNCTION(BlueprintCallable, Category = "Pipeline")
	void SerializeRootLevel() {}

	/** Serializes a sub-level if loaded */
	UFUNCTION(BlueprintCallable, Category = "Pipeline")
	void SerializeSubLevel(const FName& Level) { SerializeSubLevels({Level}); }

	/** Serializes a list of currently loaded sub-levels on the world */
	UFUNCTION(BlueprintCallable, Category = "Pipeline")
	void SerializeSubLevels(const TArray<FName>& Levels) {}

	/** Serializes all currently loaded sub-levels on the world */
	UFUNCTION(BlueprintCallable, Category = "Pipeline")
	void SerializeAllSubLevels() {}

	/** Serializes all currently loaded sub-levels on the world */
	UFUNCTION(BlueprintCallable, Category = "Pipeline")
	void SerializeAIs() {}

	/** Serializes all currently loaded sub-levels on the world */
	UFUNCTION(BlueprintCallable, Category = "Pipeline")
	void SerializePlayers() {}

	/** Serializes all currently loaded sub-levels on the world */
	UFUNCTION(BlueprintCallable, Category = "Pipeline")
	void SerializeActors(FSaveActorFilter Filter) {}

	virtual void Store(const TArray<uint8>& Bytes) {}

	void SaveToFile(const FString& FileName, const TArray<uint8>& Bytes) {}


public:

	virtual UWorld* GetWorld() const override;
	class USaveManager* GetManager() const;

	UFUNCTION(BlueprintPure, Category = "Pipeline")
	const FPipelineSettings& GetSettings() const { return Settings; }
};
