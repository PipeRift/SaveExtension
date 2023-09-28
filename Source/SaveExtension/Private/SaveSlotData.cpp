// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "SaveSlotData.h"

#include <TimerManager.h>


/////////////////////////////////////////////////////
// USaveSlotData

void USaveSlotData::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << bStoreGameInstance;
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
