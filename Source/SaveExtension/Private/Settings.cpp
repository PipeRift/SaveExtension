// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "Settings.h"

#include "SlotInfo.h"
#include "SlotData.h"


FSESettings::FSESettings()
	: SlotInfoTemplate(USlotInfo::StaticClass())
	, SlotDataTemplate(USlotData::StaticClass())
	, MaxSlots(0)
	, bAutoSave(true)
	, AutoSaveInterval(120.f)
	, bSaveOnExit(false)
	, bAutoLoad(true)
	, bDebug(false)
	, bDebugInScreen(true)

	, bStoreGameMode(true)
	, bStoreGameInstance(false)
	, bStoreLevelBlueprints(false)
	, bStoreAIControllers(false)
	, bStoreComponents(true)
	, bStoreControlRotation(true)
	, bUseCompression(true)
	, MultithreadedSerialization(ESaveASyncMode::SaveAsync)
	, FrameSplittedSerialization(ESaveASyncMode::OnlySync)
	, MaxFrameMs(5.f)
	, MultithreadedFiles(ESaveASyncMode::SaveAndLoadAsync)
	, bSaveAndLoadSublevels(true)
{}