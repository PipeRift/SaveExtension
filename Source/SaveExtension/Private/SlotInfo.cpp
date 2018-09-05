// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "SlotInfo.h"


USlotInfo::USlotInfo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Id(0)
	, TotalPlayedTime(FTimespan::Zero())
	, SlotPlayedTime(FTimespan::Zero())
	, SaveDate(FDateTime::Now())
{
}


