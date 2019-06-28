// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "Serializer.h"

void USerializer::DoSerialize(FArchive& Ar, UObject* Object)
{
	switch (Type)
	{
	case ESerializerType::CDO:
		static_cast<UCDOSerializer*>(this)->DoSerialize(Ar, Object);

	case ESerializerType::Instance:
		static_cast<UInstanceSerializer*>(this)->DoSerialize(Ar, Object);
		break;

	case ESerializerType::Blueprints:
		static_cast<UBlueprintSerializer*>(this)->DoSerialize(Ar, Object);
		break;
	}
}
