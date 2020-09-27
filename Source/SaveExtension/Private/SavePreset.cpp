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

FSaveFilter USavePreset::ToFilter() const
{
	FSaveFilter Filter{};
	Filter.FromPreset(*this);
	return Filter;
}
