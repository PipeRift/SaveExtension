// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "Serialization/Serializer.h"

#include "ControllerSerializer.generated.h"


UCLASS()
class UControllerSerializer : public UInstanceSerializer
{
	GENERATED_BODY()

protected:

	virtual void SerializeInstance(FArchive& Ar, UObject* Instance) override
	{

	}
};
