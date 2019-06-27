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

	const bool bIsSync;
	/** USE ONLY IF SYNC */
	const UWorld* const World;
	/** USE ONLY IF SYNC */
	USlotData* SlotData;

	// Locally cached settings
	const bool bStoreGameMode;
	const bool bStoreGameInstance;
	const bool bStoreLevelBlueprints;
	const bool bStoreAIControllers;
	const bool bStoreComponents;
	const bool bStoreControlRotation;


	FMTTask(const bool bInIsSync, const UWorld* InWorld, USlotData* InSlotData, const USavePreset& Preset) :
		bIsSync(bInIsSync),
		World(InWorld),
		SlotData(InSlotData),
		bStoreGameMode(Preset.bStoreGameMode),
		bStoreGameInstance(Preset.bStoreGameInstance),
		bStoreLevelBlueprints(Preset.bStoreLevelBlueprints),
		bStoreAIControllers(Preset.bStoreAIControllers),
		bStoreComponents(Preset.bStoreComponents),
		bStoreControlRotation(Preset.bStoreControlRotation)
	{}

	//Actor Tags
	FORCEINLINE bool ShouldSave(const AActor* Actor) const { return IsValid(Actor) && !HasTag(Actor, USlotDataTask::TagNoSave); }
	FORCEINLINE bool SavesTransform(const AActor* Actor) const { return Actor && Actor->IsRootComponentMovable() && !HasTag(Actor, USlotDataTask::TagNoTransform); }
	FORCEINLINE bool SavesPhysics(const AActor* Actor) const { return Actor && !HasTag(Actor, USlotDataTask::TagNoPhysics); }
	FORCEINLINE bool SavesComponents(const AActor* Actor) const { return bStoreComponents && Actor && !HasTag(Actor, USlotDataTask::TagNoComponents); }
	FORCEINLINE bool SavesTags(const AActor* Actor) const { return Actor && !HasTag(Actor, USlotDataTask::TagNoTags); }
	FORCEINLINE bool IsProcedural(const AActor* Actor) const { return Actor && Actor->HasAnyFlags(RF_WasLoaded | RF_LoadCompleted); }

	bool ShouldSaveAsWorld(const AActor* Actor, bool& bIsAIController, bool& bIsLevelScript) const;


	//Component Tags
	FORCEINLINE bool ShouldSave(const UActorComponent* Component) const
	{
		if (IsValid(Component) &&
			!HasTag(Component, USlotDataTask::TagNoSave))
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
