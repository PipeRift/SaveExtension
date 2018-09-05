// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "AssetTypeAction_SlotData.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"


//////////////////////////////////////////////////////////////////////////
// FAssetTypeAction_SavePreset

FText FAssetTypeAction_SlotData::GetName() const
{
	  return LOCTEXT("FAssetTypeAction_SlotDataName", "Save Data");
}

FColor FAssetTypeAction_SlotData::GetTypeColor() const
{
	return FColor(63, 126, 255);
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
