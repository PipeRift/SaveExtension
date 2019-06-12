// Copyright 2015-2019 Piperift. All Rights Reserved
#pragma once

#include <CoreUObject.h>
#include <Components/ActorComponent.h>

#include "ObjectFilterArchive.h"


struct FComponentFilterPacket : public FObjectFilterPacket
{
	FComponentFilterPacket() : FObjectFilterPacket{} {
		Filter.BaseClass = UActorComponent::StaticClass();
	}
};