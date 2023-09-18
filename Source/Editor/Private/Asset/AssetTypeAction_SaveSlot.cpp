// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "AssetTypeAction_SaveSlot.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"


//////////////////////////////////////////////////////////////////////////
// FAssetTypeAction_SaveSlot

FText FAssetTypeAction_SaveSlot::GetName() const
{
	return LOCTEXT("FAssetTypeAction_SaveSlotName", "Save Info");
}

FColor FAssetTypeAction_SaveSlot::GetTypeColor() const
{
	return FColor(63, 126, 255);
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
