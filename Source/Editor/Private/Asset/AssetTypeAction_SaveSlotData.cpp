// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "AssetTypeAction_SaveSlotData.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"


//////////////////////////////////////////////////////////////////////////
// FAssetTypeAction_SaveSlotData

FText FAssetTypeAction_SaveSlotData::GetName() const
{
	return LOCTEXT("FAssetTypeAction_SaveSlotDataName", "Save Data");
}

FColor FAssetTypeAction_SaveSlotData::GetTypeColor() const
{
	return FColor(63, 126, 255);
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
