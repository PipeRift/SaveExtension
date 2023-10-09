// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "SaveSlotData.h"

#include <TimerManager.h>


/////////////////////////////////////////////////////
// USaveSlotData

void USaveSlotData::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << TimeSeconds;
	Ar << GameInstance;
	RootLevel.Serialize(Ar);
	Ar << SubLevels;
}

void USaveSlotData::CleanRecords(bool bKeepSublevels)
{
	// Clean Up serialization data
	GameInstance = {};

	RootLevel.CleanRecords();
	if (!bKeepSublevels)
	{
		SubLevels.Empty();
	}
}

FPlayerRecord& USaveSlotData::FindOrAddPlayerRecord(const FUniqueNetIdRepl& UniqueId)
{
	return Players[Players.AddUnique({UniqueId})];
}

FPlayerRecord* USaveSlotData::FindPlayerRecord(const FUniqueNetIdRepl& UniqueId)
{
	const int32 Index = Players.IndexOfByPredicate([&UniqueId](const FPlayerRecord& Record) {
		return Record.UniqueId == UniqueId;
	});
	if (Index != INDEX_NONE)
	{
		return &Players[Index];
	}
	return nullptr;
}

bool USaveSlotData::FindPlayerRecord(const FUniqueNetIdRepl& UniqueId, UPARAM(Ref) FPlayerRecord& Record)
{
	if (FPlayerRecord* FoundRecord = FindPlayerRecord(UniqueId))
	{
		Record = *FoundRecord;
		return true;
	}
	return false;
}

bool USaveSlotData::RemovePlayerRecord(const FUniqueNetIdRepl& UniqueId)
{
	return Players.RemoveAll([&UniqueId](const FPlayerRecord& Record){
		return Record.UniqueId == UniqueId;
	}) > 0;
}
