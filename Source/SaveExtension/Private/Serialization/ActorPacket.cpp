// Copyright 2015-2019 Piperift. All Rights Reserved

#include "ActorPacket.h"


bool FActorPacketRecord::operator==(const FActorPacketRecord& Other) const
{
	return Filter == Other.Filter; // #TODO: Compare custom serializer too
}

bool FActorPacketSettings::IsLevelAllowed(FName Name) const
{
	// #NOTE: Make sure names are the same
	return LevelList.ContainsByPredicate([Name](const auto& Level) {
		const FName ObjName = Level->GetFName();
		return Level.IsValid() && ObjName == Name;
	});
}
