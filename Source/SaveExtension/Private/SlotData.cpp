// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "SlotData.h"
#include <TimerManager.h>

#include "SavePreset.h"


/////////////////////////////////////////////////////
// USlotData

void USlotData::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << GameInstance;
	Ar << GameMode;
	Ar << GameState;

	Ar << PlayerPawn;
	Ar << PlayerController;
	Ar << PlayerState;
	Ar << PlayerHUD;

	MainLevel.Serialize(Ar);
	Ar << SubLevels;
}

void USlotData::Clean(bool bKeepLevels)
{
	//Clean Up serialization data
	GameMode = {};
	GameState = {};

	PlayerPawn = {};
	PlayerController = {};
	PlayerState = {};
	PlayerHUD = {};

	GameInstance = {};

	MainLevel.Clean();

	if (!bKeepLevels)
	{
		SubLevels.Empty();
	}
}
