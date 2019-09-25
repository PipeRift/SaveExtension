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


	FMTTask(const bool bIsloading, const UWorld* InWorld, USlotData* InSlotData, const FSaveFilter& Filter)
		: World(InWorld)
		, SlotData(InSlotData)
		, Filter(Filter)
	{}
};
