// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "SavePreset.h"

#include "LevelFilter.h"
#include "SlotData.h"
#include "SlotInfo.h"


USavePreset::USavePreset()
	: Super()
	, SlotInfoClass(USlotInfo::StaticClass())
	, SlotDataClass(USlotData::StaticClass())
{}

void USavePreset::BPGetSlotNameFromId_Implementation(int32 Id, FName& Name) const
{
	// Call C++ inheritance chain by default
	return GetSlotNameFromId(Id, Name);
}

void USavePreset::GetSlotNameFromId(int32 Id, FName& Name) const
{
	if (IsValidId(Id))
	{
		Name = { FString::FromInt(Id) };
	}
}

FSELevelFilter USavePreset::ToFilter() const
{
	FSELevelFilter Filter{};
	Filter.FromPreset(*this);
	return Filter;
}
