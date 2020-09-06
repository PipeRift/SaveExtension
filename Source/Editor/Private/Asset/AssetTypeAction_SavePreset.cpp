// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "AssetTypeAction_SavePreset.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"


//////////////////////////////////////////////////////////////////////////
// FAssetTypeAction_SavePreset

FText FAssetTypeAction_SavePreset::GetName() const
{
	  return LOCTEXT("FAssetTypeAction_SavePresetName", "Preset");
}

FColor FAssetTypeAction_SavePreset::GetTypeColor() const
{
	return FColor(178, 7, 58);
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE