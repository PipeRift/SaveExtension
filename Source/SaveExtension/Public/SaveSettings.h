// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "SaveSlot.h"

#include <Engine/DeveloperSettings.h>

#include "SaveSettings.generated.h"


UCLASS(ClassGroup = SaveExtension, defaultconfig, config = Game, meta = (DisplayName = "Save Extension"))
class SAVEEXTENSION_API USaveSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// Active Slot classes are initialized as active slots.
	// Active saves represent an always valid save game in memory that can be filled periodically or
	// saved.
	UPROPERTY(EditAnywhere, Category = "Save Extension", Config)
	TSubclassOf<USaveSlot> ActiveSlot;

	// If true SaveManager will tick with the world, otherwise with the engine. If true, the saving process
	// may be interrupted if the game is paused.
	UPROPERTY(EditAnywhere, Category = "Save Extension", Config)
	bool bTickWithGameWorld = false;
};
