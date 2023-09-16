// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "SavePreset.h"

#include "LevelFilter.h"
#include "SaveSlot.h"
#include "SaveSlotData.h"


void USavePreset::BPGetSlotNameFromId_Implementation(int32 Id, FName& Name) const
{
	// Call C++ inheritance chain by default
	return GetSlotNameFromId(Id, Name);
}

FSELevelFilter USavePreset::ToFilter() const
{
	FSELevelFilter Filter{};
	Filter.FromPreset(*this);
	return Filter;
}
