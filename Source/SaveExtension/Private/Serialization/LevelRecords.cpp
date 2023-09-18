// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Serialization/LevelRecords.h"

#include "SaveSlotData.h"


/////////////////////////////////////////////////////
// LevelRecords

const FName FPersistentLevelRecord::PersistentName{"Persistent"};


bool FLevelRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << bOverrideGlobalFilter;
	if (bOverrideGlobalFilter)
	{
		static UScriptStruct* const LevelFilterType{FSELevelFilter::StaticStruct()};
		LevelFilterType->SerializeItem(Ar, &Filter, nullptr);
	}

	Ar << LevelScript;
	Ar << Actors;

	return true;
}

void FLevelRecord::CleanRecords()
{
	LevelScript = {};
	Actors.Empty();
}
