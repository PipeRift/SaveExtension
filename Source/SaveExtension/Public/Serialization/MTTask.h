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


/////////////////////////////////////////////////////
// FSlotDataActorsTask
// Async task to serialize actors from a level.
class FMTTask : public FNonAbandonableTask {
protected:

	/** Used only if Sync */
	const UWorld* const World;
	USlotData* SlotData;

	// Locally cached settings
	const FActorClassFilter* ActorFilter;
	const FComponentClassFilter* ComponentFilter;
	const bool bStoreGameInstance;
	const bool bStoreComponents;


	FMTTask(const bool bIsloading, const UWorld* InWorld, USlotData* InSlotData, const USavePreset& Preset)
		: World(InWorld)
		, SlotData(InSlotData)
		, ActorFilter(&Preset.GetActorFilter(bIsloading))
		, ComponentFilter(&Preset.GetComponentFilter(bIsloading))
		, bStoreGameInstance(Preset.bStoreGameInstance)
		, bStoreComponents(Preset.bStoreComponents)
	{}

	//Actor Tags
	FORCEINLINE bool ShouldSave(const AActor* Actor) const { return IsValid(Actor) && !HasTag(Actor, USlotDataTask::TagNoSave); }
	FORCEINLINE bool SavesTransform(const AActor* Actor) const { return Actor && Actor->IsRootComponentMovable() && !HasTag(Actor, USlotDataTask::TagNoTransform); }
	FORCEINLINE bool SavesPhysics(const AActor* Actor) const { return Actor && !HasTag(Actor, USlotDataTask::TagNoPhysics); }
	FORCEINLINE bool SavesComponents(const AActor* Actor) const { return bStoreComponents && Actor && !HasTag(Actor, USlotDataTask::TagNoComponents); }
	FORCEINLINE bool SavesTags(const AActor* Actor) const { return Actor && !HasTag(Actor, USlotDataTask::TagNoTags); }
	FORCEINLINE bool IsProcedural(const AActor* Actor) const { return Actor && Actor->HasAnyFlags(RF_WasLoaded | RF_LoadCompleted); }

	FORCEINLINE bool ShouldSaveAsWorld(const AActor* Actor) const
	{
		return ActorFilter->IsClassAllowed(Actor->GetClass());
	}


	//Component Tags
	FORCEINLINE bool ShouldSave(const UActorComponent* Component) const
	{
		if (IsValid(Component))
		{
			return ComponentFilter->IsClassAllowed(Component->GetClass())
				&& !HasTag(Component, USlotDataTask::TagNoSave);
		}
		return false;
	}

	bool SavesTransform(const UActorComponent* Component) const
	{
		return Component &&
			Component->GetClass()->IsChildOf<USceneComponent>() &&
			HasTag(Component, USlotDataTask::TagTransform);
	}
	FORCEINLINE bool SavesTags(const UActorComponent* Component) const { return Component && !HasTag(Component, USlotDataTask::TagNoTags); }

private:

	static FORCEINLINE bool HasTag(const AActor* Actor, const FName Tag)
	{
		return Actor->ActorHasTag(Tag);
	}

	static FORCEINLINE bool HasTag(const UActorComponent* Component, const FName Tag)
	{
		return Component->ComponentHasTag(Tag);
	}
};
