// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "LevelRecords.h"
#include "SlotData.h"


/////////////////////////////////////////////////////
// LevelRecords

FName FPersistentLevelRecord::PersistentName{ "Persistent" };


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
