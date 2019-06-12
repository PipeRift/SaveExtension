// Copyright 2015-2019 Piperift. All Rights Reserved
#pragma once

#include <CoreUObject.h>
#include <GameFramework/Actor.h>

#include "ObjectFilterArchive.h"


struct FActorFilterPacket : public FObjectFilterPacket
{
	FActorFilterPacket() : FObjectFilterPacket{} {
		Filter.BaseClass = AActor::StaticClass();
	}
};