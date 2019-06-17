// Copyright 2015-2019 Piperift. All Rights Reserved
#pragma once

#include <CoreUObject.h>
#include <Components/ActorComponent.h>

#include "Records.h"
#include "ObjectPacket.h"
#include "ComponentPacket.generated.h"


USTRUCT()
struct FComponentPacketRecord : public FObjectPacketRecord
{
	GENERATED_BODY()

	// Component packets don't have a list of components like ActorPackets
	// because its more efficient to store and read them from ActorRecords

	FComponentPacketRecord() : Super()
	{
		Filter.BaseClass = UActorComponent::StaticClass();
	}
};