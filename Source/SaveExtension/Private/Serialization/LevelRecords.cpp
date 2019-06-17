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

FActorPacketRecord& FLevelRecord::FindOrCreateActorPacket(const FActorPacketRecord& NewPacket)
{
	int32 Index = ActorPackets.Find(NewPacket);
	if (Index != INDEX_NONE)
	{
		return ActorPackets[Index];
	}
	return ActorPackets.Add_GetRef(NewPacket);
}
