// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "AssetTypeAction_SavePreset.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"


//////////////////////////////////////////////////////////////////////////
// FAssetTypeAction_SavePreset

FText FAssetTypeAction_SavePreset::GetName() const
{
	  return LOCTEXT("FAssetTypeAction_SavePresetName", "Save Preset");
}

FColor FAssetTypeAction_SavePreset::GetTypeColor() const
{
	return FColor(63, 126, 255);
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
