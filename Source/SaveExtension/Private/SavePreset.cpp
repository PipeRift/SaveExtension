// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "SavePreset.h"
#include "SlotInfo.h"
#include "SlotData.h"


USavePreset::USavePreset()
	: Super()
	, SlotInfoTemplate(USlotInfo::StaticClass())
	, SlotDataTemplate(USlotData::StaticClass())
	, MaxSlots(0)
	, bAutoSave(true)
	, AutoSaveInterval(120.f)
	, bSaveOnExit(false)
	, bAutoLoad(true)
	, bDebug(false)
	, bDebugInScreen(true)

	, bUseCompression(true)
	, bStoreGameInstance(false)
	, bUseLoadActorFilter(false)
	, bStoreComponents(true)
	, bUseLoadComponentFilter(false)
	, MultithreadedSerialization(ESaveASyncMode::SaveAsync)
	, FrameSplittedSerialization(ESaveASyncMode::OnlySync)
	, MaxFrameMs(5.f)
	, MultithreadedFiles(ESaveASyncMode::SaveAndLoadAsync)
	, bSaveAndLoadSublevels(true)
{}
