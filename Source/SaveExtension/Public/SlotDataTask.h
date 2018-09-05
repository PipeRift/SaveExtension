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


	static const FName TagNoSave;
	static const FName TagTransform;
	static const FName TagNoTransform;
	static const FName TagNoPhysics;
	static const FName TagNoComponents;
	static const FName TagNoTags;


	bool bRunning;

protected:

	UPROPERTY()
	USlotData* SlotData;

	UPROPERTY()
	UWorld* World;

	UPROPERTY()
	const USavePreset* Preset;


public:

	USlotDataTask() : Super(), bRunning(false) {}

	void Prepare(USlotData* InSaveData, UWorld* InWorld, const USavePreset* InPreset)
	{
		SlotData = InSaveData;
		World = InWorld;
		Preset = InPreset;
	}

	void Start();

	virtual void Tick(float DeltaTime) {}

	void Finish();

	bool IsRunning() const { return bRunning; }

	virtual void OnTick(float DeltaTime) {}

protected:

	virtual void OnStart() {}

	virtual void OnFinish() {}

	USaveManager* GetManager() const {
		return Cast<USaveManager>(GetOuter());
	}

	//~ Begin UObject Interface
	virtual UWorld* GetWorld() const override;
	//~ End UObject Interface


	//Actor Tags
	FORCEINLINE bool ShouldSave(const AActor* Actor)	 { return IsValid(Actor) && !HasTag(Actor, TagNoSave);	   }
	FORCEINLINE bool SavesTransform(const AActor* Actor) { return Actor && Actor->IsRootComponentMovable() && !HasTag(Actor, TagNoTransform);  }
	FORCEINLINE bool SavesPhysics(const AActor* Actor)	 { return Actor && !HasTag(Actor, TagNoPhysics);	}
	FORCEINLINE bool SavesComponents(const AActor* Actor) { return Preset->bStoreComponents && Actor && !HasTag(Actor, TagNoComponents); }
	FORCEINLINE bool CanBeKilled(const AActor* Actor)	 { return Actor && !HasTag(Actor, TEXT("!Kill"));	   }
	FORCEINLINE bool SavesTags(const AActor* Actor)	     { return Actor && !HasTag(Actor, TagNoTags);	   }
	FORCEINLINE bool IsProcedural(const AActor* Actor)   { return Actor &&  Actor->HasAnyFlags(RF_WasLoaded | RF_LoadCompleted); }


	bool ShouldSaveAsWorld(const AActor* Actor);

	//Component Tags
	FORCEINLINE bool ShouldSave(const UActorComponent* Component) { return IsValid(Component) && !HasTag(Component, TagNoSave) && Component->GetClass()->IsChildOf<UStaticMeshComponent>(); }
	bool SavesTransform(const UActorComponent* Component) {
		return Component &&
			   Component->GetClass()->IsChildOf<USceneComponent>() &&
			   HasTag(Component, TagTransform);
	}
	FORCEINLINE bool SavesTags(const UActorComponent* Component)  { return Component && !HasTag(Component, TagNoTags); }

private:

	static FORCEINLINE bool HasTag(const AActor* Actor, const FName Tag) {
		check(Actor);
		return Actor->ActorHasTag(Tag);
	}

	static FORCEINLINE bool HasTag(const UActorComponent* Component, const FName Tag) {
		check(Component);
		return Component->ComponentHasTag(Tag);
	}

};
