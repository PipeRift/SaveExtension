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

FPlayerRecord* USaveSlotData::FindPlayerRecord(const APlayerState* Player)
{
	if (!Player)
	{
		return nullptr;
	}

	const int32 Index = Players.IndexOfByPredicate([Player](const FPlayerRecord& Record) {
		return Record == *Player;
	});
	if (Index != INDEX_NONE)
	{
		return &Players[Index];
	}
	return nullptr;
}

bool USaveSlotData::FindPlayerRecord(const APlayerState* Player, FPlayerRecord& Record)
{
	if (FPlayerRecord* FoundRecord = FindPlayerRecord(Player))
	{
		Record = *FoundRecord;
		return true;
	}
	return false;
}

bool USaveSlotData::RemovePlayerRecord(const APlayerState* Player)
{
	if (!Player)
	{
		return false;
	}
	return Players.RemoveAll([Player](const FPlayerRecord& Record) {
		return Record == *Player;
	}) > 0;
}
