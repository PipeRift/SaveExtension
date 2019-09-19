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
	FSaveFilter Filter;
	const bool bStoreGameInstance;
	const bool bStoreComponents;


	FMTTask(const bool bIsloading, const UWorld* InWorld, USlotData* InSlotData, const FSESettings& Preset)
		: World(InWorld)
		, SlotData(InSlotData)
		, ActorFilter(&Preset.GetActorFilter(bIsloading))
		, ComponentFilter(&Preset.GetComponentFilter(bIsloading))
		, bStoreGameInstance(Preset.bStoreGameInstance)
		, bStoreComponents(Preset.bStoreComponents)
	{}
};
