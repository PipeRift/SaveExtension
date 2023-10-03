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
public:
	/** Used only if Sync */
	UWorld* const World = nullptr;
	USaveSlotData* SlotData = nullptr;


	FMTTask(const bool bIsloading, UWorld* World, USaveSlotData* SlotData)
		: World(World)
		, SlotData(SlotData)
	{}
};
