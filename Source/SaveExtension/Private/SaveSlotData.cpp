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

	static UScriptStruct* const LevelFilterType{FSELevelFilter::StaticStruct()};
	LevelFilterType->SerializeItem(Ar, &GlobalLevelFilter, nullptr);
	MainLevel.Serialize(Ar);
	Ar << SubLevels;
}

void USaveSlotData::CleanRecords(bool bKeepSublevels)
{
	// Clean Up serialization data
	GameInstance = {};

	MainLevel.CleanRecords();
	if (!bKeepSublevels)
	{
		SubLevels.Empty();
	}
}
