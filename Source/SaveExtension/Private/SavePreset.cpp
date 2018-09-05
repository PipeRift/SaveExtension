// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "SavePreset.h"
#include "SlotInfo.h"
#include "SlotData.h" 


USavePreset::USavePreset()
	: Super(),
	SlotInfoTemplate(USlotInfo::StaticClass()),
	SlotDataTemplate(USlotData::StaticClass()),
	MaxSlots(0),
	bAutoSave(true),
	AutoSaveInterval(120.f),
	bSaveOnExit(false),
	bAutoLoad(true),
	bDebug(false),
	bDebugInScreen(true),

	bStoreGameMode(true),
	bStoreGameInstance(false),
	bStoreLevelBlueprints(false),
	bStoreAIControllers(false),
	bStoreComponents(true),
	bStoreControlRotation(true)
{}