// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "ISaveExtension.h"

#include <Engine/Level.h>
#include <Engine/LevelStreaming.h>
#include <GameFramework/Actor.h>
#include <Engine/LevelScriptActor.h>
#include <GameFramework/Controller.h>
#include <AIController.h>
#include <Components/StaticMeshComponent.h>
#include <Components/SkeletalMeshComponent.h>

#include "SavePreset.h"
#include "SlotData.h"

#include "SlotDataTask.generated.h"

class USaveManager;


/**
* Base class for managing a SaveData file
*/
UCLASS()
class USlotDataTask : public UObject
{
	GENERATED_BODY()

public:

	static const FName TagNoSave;
	static const FName TagNoTransform;
	static const FName TagNoComponents;
	static const FName TagNoPhysics;
	static const FName TagNoTags;
	static const FName TagTransform;

	static bool IsSaveTag(const FName& Tag);

	//Actor Tags
	static FORCEINLINE bool ShouldSave(const AActor* Actor)     { return IsValid(Actor) && !HasTag(Actor, TagNoSave); }
	static FORCEINLINE bool SavesTransform(const AActor* Actor) { return Actor && Actor->IsRootComponentMovable() && !HasTag(Actor, TagNoTransform); }
	static FORCEINLINE bool SavesPhysics(const AActor* Actor)   { return Actor && !HasTag(Actor, TagNoPhysics); }
	static FORCEINLINE bool SavesTags(const AActor* Actor)      { return Actor && !HasTag(Actor, TagNoTags); }
	static FORCEINLINE bool IsProcedural(const AActor* Actor)   { return Actor && Actor->HasAnyFlags(RF_WasLoaded | RF_LoadCompleted); }
	FORCEINLINE bool SavesComponents(const AActor* Actor) const { return Preset->bStoreComponents && Actor && !HasTag(Actor, TagNoComponents); }

	static FORCEINLINE bool HasTag(const AActor* Actor, const FName Tag) {
		return Actor->ActorHasTag(Tag);
	}

	static FORCEINLINE bool HasTag(const UActorComponent* Component, const FName Tag) {
		return Component->ComponentHasTag(Tag);
	}

private:

	uint8 bRunning : 1;
	uint8 bFinished : 1;
	uint8 bSucceeded : 1;

protected:

	UPROPERTY()
	USlotData* SlotData;

	UPROPERTY()
	const USavePreset* Preset;

	// Cached value from preset to avoid cache misses
	float MaxFrameMs;

	bool bIsLoading = false;
	const FClassFilter* ActorFilter;


public:

	USlotDataTask()
		: Super()
		, bRunning(false)
		, bFinished(false)
		, bIsLoading(false)
		, ActorFilter(nullptr)
	{}

	void Prepare(USlotData* InSaveData, const USavePreset* InPreset)
	{
		SlotData = InSaveData;
		Preset = InPreset;
		MaxFrameMs = Preset? Preset->GetMaxFrameMs() : 5.f;
		ActorFilter = &Preset->GetActorFilter(bIsLoading);
		ActorFilter->BakeAllowedClasses();
	}

	USlotDataTask* Start();

	virtual void Tick(float DeltaTime) {}

	void Finish(bool bSuccess);

	bool IsRunning() const  { return bRunning;  }
	bool IsFinished() const { return bFinished; }
	bool IsSucceeded() const  { return IsFinished() && bSucceeded;  }
	bool IsFailed() const     { return IsFinished() && !bSucceeded; }
	bool IsScheduled() const;

	virtual void OnTick(float DeltaTime) {}

protected:

	virtual void OnStart() {}

	virtual void OnFinish(bool bSuccess) {}

	USaveManager* GetManager() const;

	//~ Begin UObject Interface
	virtual UWorld* GetWorld() const override;
	//~ End UObject Interface

	FORCEINLINE bool ShouldSaveAsWorld(const AActor* Actor) const
	{
		return ActorFilter->IsClassAllowed(Actor->GetClass());
	}

	//Component Tags
	FORCEINLINE bool ShouldSave(const UActorComponent* Component) const
	{
		if (IsValid(Component) && !HasTag(Component, TagNoSave))
		{
			const UClass* const Class = Component->GetClass();
			return !Class->IsChildOf<UStaticMeshComponent>() &&
				   !Class->IsChildOf<USkeletalMeshComponent>();
		}
		return false;
	}

	bool SavesTransform(const UActorComponent* Component) const
	{
		return Component &&
			   Component->GetClass()->IsChildOf<USceneComponent>() &&
			   HasTag(Component, TagTransform);
	}

	FORCEINLINE bool SavesTags(const UActorComponent* Component) const
	{
		return Component && !HasTag(Component, TagNoTags);
	}


protected:

	FORCEINLINE float GetTimeMilliseconds() const {
		return FPlatformTime::ToMilliseconds(FPlatformTime::Cycles());
	}
};
