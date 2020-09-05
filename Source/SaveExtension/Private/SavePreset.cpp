// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "SavePreset.h"
#include "SlotInfo.h"
#include "SlotData.h"


USavePreset::USavePreset()
	: Super()
	, SlotInfoTemplate(USlotInfo::StaticClass())
	, SlotDataTemplate(USlotData::StaticClass())
{}
