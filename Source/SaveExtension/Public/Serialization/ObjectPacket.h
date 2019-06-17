// Copyright 2015-2019 Piperift. All Rights Reserved
#pragma once

#include <CoreMinimal.h>

#include "ClassFilter.h"
#include "ObjectPacket.generated.h"


USTRUCT()
struct FObjectPacketRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FClassFilter Filter;


	FObjectPacketRecord() = default;
	FObjectPacketRecord(const FClassFilter& InFilter) : Filter(InFilter) {}
};
