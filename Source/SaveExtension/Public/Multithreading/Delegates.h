// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>


DECLARE_DELEGATE_OneParam(FOnSlotsLoaded, const TArray<class USaveSlot*>&);

// @param Amount of slots removed
DECLARE_DELEGATE(FOnSlotsDeleted);
