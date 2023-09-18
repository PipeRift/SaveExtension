// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "ISaveExtension.h"
#include "LevelFilter.h"

#include <Engine/Level.h>
#include <Engine/LevelStreaming.h>
#include <GameFramework/Actor.h>
#include <GameFramework/Controller.h>


/////////////////////////////////////////////////////
// FSlotDataActorsTask
// Async task to serialize actors from a level.
class FMTTask : public FNonAbandonableTask
{
protected:
	/** Used only if Sync */
	const UWorld* const World;
	USaveSlotData* SlotData;

	// Locally cached settings
	const FSELevelFilter& Filter;


	FMTTask(
		const bool bIsloading, const UWorld* InWorld, USaveSlotData* InSlotData, const FSELevelFilter& Filter)
		: World(InWorld)
		, SlotData(InSlotData)
		, Filter(Filter)
	{}
};
