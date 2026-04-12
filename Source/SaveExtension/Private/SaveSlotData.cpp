// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "SaveSlotData.h"

#include <GameFramework/OnlineReplStructs.h>
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

	uint8 NumPlayers = Players.Num();
	Ar << NumPlayers;
	if (Ar.IsSaving())
	{
		for (int32 i = 0; i < NumPlayers; ++i)
		{
			Players[i].Serialize(Ar);
		}
	}
	if (Ar.IsLoading())
	{
		Players.Empty(NumPlayers);
		for (int32 i = 0; i < NumPlayers; ++i)
		{
			FPlayerRecord PlayerRecord;
			PlayerRecord.Serialize(Ar);
			Players.Add(PlayerRecord);
		}
	}
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
	return Players[Players.AddUnique(FPlayerRecord(UniqueId.ToString()))];
}

FPlayerRecord* USaveSlotData::FindPlayerRecord(const FUniqueNetIdRepl& UniqueId)
{
	const FString UniqueIdStr = UniqueId.ToString();
	const int32 Index = Players.IndexOfByPredicate([&UniqueIdStr](const FPlayerRecord& Record) {
		return Record.UniqueId == UniqueIdStr;
	});
	if (Index != INDEX_NONE)
	{
		return &Players[Index];
	}
	return nullptr;
}

bool USaveSlotData::FindPlayerRecord(const FUniqueNetIdRepl& UniqueId, FPlayerRecord& Record)
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
	const FString UniqueIdStr = UniqueId.ToString();
	return Players.RemoveAll([&UniqueIdStr](const FPlayerRecord& Record) {
		return Record.UniqueId == UniqueIdStr;
	}) > 0;
}
