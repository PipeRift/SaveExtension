// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "SavePreset.h"

#include "SaveFilter.h"
#include "SlotData.h"
#include "SlotInfo.h"


USavePreset::USavePreset()
	: Super()
	, SlotInfoTemplate(USlotInfo::StaticClass())
	, SlotDataTemplate(USlotData::StaticClass())
{}

void USavePreset::BPGenerateSlotName_Implementation(int32 Id, FString& Name) const
{
	// Call C++ inheritance chain by default
	return GenerateSlotName(Id, Name);
}

void USavePreset::GenerateSlotName(int32 Id, FString& Name) const
{
	if (IsValidId(Id))
	{
		Name = FString::FromInt(Id);
	}
}

FSaveFilter USavePreset::ToFilter() const
{
	FSaveFilter Filter{};
	Filter.FromPreset(*this);
	return Filter;
}
