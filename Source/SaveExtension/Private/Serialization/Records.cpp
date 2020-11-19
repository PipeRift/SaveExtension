// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "Serialization/Records.h"
#include "SlotData.h"


/////////////////////////////////////////////////////
// Records

bool FBaseRecord::Serialize(FArchive& Ar)
{
	Ar << Name;
	return true;
}

FObjectRecord::FObjectRecord(const UObject* Object) : Super()
{
	Class = nullptr;
	if (Object)
	{
		Name = Object->GetFName();
		Class = Object->GetClass();
	}
}

bool FObjectRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (!Name.IsNone())
		Ar << Class;
	else if (Ar.IsLoading())
		Class = nullptr;

	if (Class)
	{
		Ar << Data;
		Ar << Tags;
	}
	return true;
}

bool FComponentRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (Class)
	{
		Ar << Transform;
	}
	return true;
}

bool FActorRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (!Class)
	{
		return true;
	}

	Ar.SerializeBits(&bHiddenInGame, 1);
	Ar.SerializeBits(&bIsProcedural, 1);

	Ar << Transform;

	// Reduce memory footprint to 1 bool if not moving
	bool bIsMoving = Ar.IsSaving() && (!LinearVelocity.IsNearlyZero() || !AngularVelocity.IsNearlyZero());
	Ar << bIsMoving;
	if(bIsMoving)
	{
		Ar << LinearVelocity;
		Ar << AngularVelocity;
	}
	Ar << ComponentRecords;
	return true;
}
