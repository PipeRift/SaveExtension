// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "ISaveExtension.h"

#include <Engine/Level.h>
#include <Engine/LevelStreaming.h>
#include <GameFramework/Actor.h>
#include <Engine/LevelScriptActor.h>
#include <GameFramework/Controller.h>
#include <AIController.h>

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

private:

	uint8 bRunning : 1;
	uint8 bFinished : 1;
	uint8 bSuccess : 1;

protected:

	UPROPERTY()
	USlotData* SlotData;

	UPROPERTY()
	UWorld* World;

	UPROPERTY()
	const USavePreset* Preset;

	// Cached value from preset to avoid cache misses
	float MaxFrameMs;


public:

	USlotDataTask() : Super(), bRunning(false), bFinished(false) {}

	void Prepare(USlotData* InSaveData, UWorld* InWorld, const USavePreset* InPreset)
	{
		SlotData = InSaveData;
		World = InWorld;
		Preset = InPreset;
		MaxFrameMs = Preset? Preset->GetMaxFrameMs() : 5.f;
	}

	USlotDataTask* Start();

	virtual void Tick(float DeltaTime) {}

	void Finish(bool bInSuccess);

	bool IsRunning() const  { return bRunning;  }
	bool IsFinished() const { return bFinished; }
	bool IsSucceeded() const  { return IsFinished() && bSuccess;  }
	bool IsFailed() const     { return IsFinished() && !bSuccess; }
	bool IsScheduled() const;

	virtual void OnTick(float DeltaTime) {}

protected:

	virtual void OnStart() {}

	virtual void OnFinish() {}

	USaveManager* GetManager() const;

	//~ Begin UObject Interface
	virtual UWorld* GetWorld() const override;
	//~ End UObject Interface


	//Actor Tags
	FORCEINLINE bool ShouldSave(const AActor* Actor) const     { return IsValid(Actor) && !HasTag(Actor, TagNoSave);	   }
	FORCEINLINE bool SavesTransform(const AActor* Actor) const { return Actor && Actor->IsRootComponentMovable() && !HasTag(Actor, TagNoTransform);  }
	FORCEINLINE bool SavesPhysics(const AActor* Actor) const   { return Actor && !HasTag(Actor, TagNoPhysics);	}
	FORCEINLINE bool SavesComponents(const AActor* Actor) const { return Preset->bStoreComponents && Actor && !HasTag(Actor, TagNoComponents); }
	FORCEINLINE bool SavesTags(const AActor* Actor) const      { return Actor && !HasTag(Actor, TagNoTags);	   }
	FORCEINLINE bool IsProcedural(const AActor* Actor) const   { return Actor &&  Actor->HasAnyFlags(RF_WasLoaded | RF_LoadCompleted); }


	bool ShouldSaveAsWorld(const AActor* Actor) const;

	//Component Tags
	FORCEINLINE bool ShouldSave(const UActorComponent* Component) const {
		if (IsValid(Component) &&
		   !HasTag(Component, TagNoSave))
		{
			const UClass* const Class = Component->GetClass();
			return !Class->IsChildOf<UStaticMeshComponent>() &&
				   !Class->IsChildOf<USkeletalMeshComponent>();
		}
		return false;
	}

	bool SavesTransform(const UActorComponent* Component) const {
		return Component &&
			   Component->GetClass()->IsChildOf<USceneComponent>() &&
			   HasTag(Component, TagTransform);
	}
	FORCEINLINE bool SavesTags(const UActorComponent* Component) const { return Component && !HasTag(Component, TagNoTags); }

private:

	static FORCEINLINE bool HasTag(const AActor* Actor, const FName Tag) {
		check(Actor);
		return Actor->ActorHasTag(Tag);
	}

	static FORCEINLINE bool HasTag(const UActorComponent* Component, const FName Tag) {
		check(Component);
		return Component->ComponentHasTag(Tag);
	}


protected:

	FORCEINLINE float GetTimeMilliseconds() const {
		return FPlatformTime::ToMilliseconds(FPlatformTime::Cycles());
	}
};
