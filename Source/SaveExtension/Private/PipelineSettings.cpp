// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "PipelineSettings.h"

#include <Engine/StaticMeshActor.h>
#include <InstancedFoliageActor.h>
#include <Engine/ReflectionCapture.h>
#include <Engine/LODActor.h>
#include <Engine/Brush.h>
#include <Lightmass/LightmassPortal.h>
#include <GameFramework/GameMode.h>
#include <GameFramework/GameState.h>
#include <GameFramework/PlayerState.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/HUD.h>
#include <NavigationData.h>

#include "SlotInfo.h"
#include "SlotData.h"


FSettingsActorFilter::FSettingsActorFilter()
	: IgnoredChildren {
		AStaticMeshActor::StaticClass(),
		AInstancedFoliageActor::StaticClass(),
		AReflectionCapture::StaticClass(),
		APlayerController::StaticClass(),
		ALightmassPortal::StaticClass(),
		ANavigationData::StaticClass(),
		APlayerState::StaticClass(),
		AGameState::StaticClass(),
		AGameMode::StaticClass(),
		ALODActor::StaticClass(),
		ABrush::StaticClass(),
		AHUD::StaticClass()
	}
{}


void FSettingsActorFilter::BuildTree()
{
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* const Class = *It;

		// Iterate parent classes
		UClass* CurrentClass = Class;
		while (CurrentClass)
		{
			if (AllowedChildren.Contains(CurrentClass))
			{
				// First allowed class marks it as allowed
				BakedAllowedClasses.Add(Class);
				break;
			}
			else if (IgnoredChildren.Contains(CurrentClass))
			{
				// First ignored class marks it as not allowed
				break;
			}

			CurrentClass = CurrentClass->GetSuperClass();
		}
	}
}

FPipelineSettings::FPipelineSettings()
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