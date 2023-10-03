// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "SaveSlot.h"


/** Called when game has been saved
 * @param SaveSlot the saved slot. Null if save failed
 */
DECLARE_DELEGATE_OneParam(FOnGameSaved, USaveSlot*);

/** Called when game has been loaded
 * @param SaveSlot the loaded slot. Null if load failed
 */
DECLARE_DELEGATE_OneParam(FOnGameLoaded, USaveSlot*);
