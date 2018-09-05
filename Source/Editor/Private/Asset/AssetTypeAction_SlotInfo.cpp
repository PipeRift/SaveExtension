// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "AssetTypeAction_SlotInfo.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"


//////////////////////////////////////////////////////////////////////////
// FAssetTypeAction_SavePreset

FText FAssetTypeAction_SlotInfo::GetName() const
{
	  return LOCTEXT("FAssetTypeAction_SlotInfoName", "Save Info");
}

FColor FAssetTypeAction_SlotInfo::GetTypeColor() const
{
	return FColor(63, 126, 255);
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
