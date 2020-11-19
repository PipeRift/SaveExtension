// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>


DECLARE_DELEGATE_OneParam(FOnSlotInfosLoaded, const TArray<class USlotInfo*>&);

// @param Amount of slots removed
DECLARE_DELEGATE(FOnSlotsDeleted);
