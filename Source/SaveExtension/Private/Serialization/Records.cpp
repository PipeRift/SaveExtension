// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "Records.h"
#include "SlotData.h"


FName FPersistentLevelRecord::PersistentName{ "Persistent" };


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

	Ar << Data;
	Ar << Tags;
	return true;
}

bool FComponentRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << Transform;
	return true;
}

bool FActorRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.SerializeBits(&bHiddenInGame, 1);
	Ar.SerializeBits(&bIsProcedural, 1);

	Ar << Transform;
	Ar << LinearVelocity;
	Ar << AngularVelocity;
	Ar << ComponentRecords;
	return true;
}

bool FControllerRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << ControlRotation;

	return true;
}


bool FLevelRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << LevelScript;
	Ar << Actors;
	Ar << AIControllers;

	return true;
}

void FLevelRecord::Clean()
{
	LevelScript = {};
	Actors.Empty();
	AIControllers.Empty();
}
