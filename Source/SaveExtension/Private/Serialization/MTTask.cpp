// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "MTTask.h"

#include <Engine/StaticMeshActor.h>
#include <Engine/ReflectionCapture.h>
#include <Engine/LODActor.h>
#include <Engine/Brush.h>
#include <Lightmass/LightmassPortal.h>
#include <GameFramework/GameMode.h>
#include <GameFramework/GameState.h>
#include <GameFramework/PlayerState.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/HUD.h>
#include <InstancedFoliageActor.h>

#include "SaveManager.h"
#include "SavePreset.h"


/////////////////////////////////////////////////////
// FSlotDataActorsTask

bool FMTTask::ShouldSaveAsWorld(const AActor* Actor, bool& bIsAIController, bool& bIsLevelScript) const
{
	const UClass* const ActorClass = Actor->GetClass();

	bIsAIController = ActorClass->IsChildOf<AAIController>();
	if (bIsAIController)
	{
		return bStoreAIControllers;
	}

	bIsLevelScript = ActorClass->IsChildOf<ALevelScriptActor>();
	if (bIsLevelScript)
	{
		return bStoreLevelBlueprints;
	}

	if (ActorClass == AStaticMeshActor::StaticClass() ||
		ActorClass->IsChildOf<AInstancedFoliageActor>() ||
		ActorClass->IsChildOf<AReflectionCapture>() ||
		ActorClass->IsChildOf<APlayerController>() ||
		ActorClass->IsChildOf<ALightmassPortal>() ||
		ActorClass->IsChildOf<ANavigationData>() ||
		ActorClass->IsChildOf<APlayerState>() ||
		ActorClass->IsChildOf<AGameState>() ||
		ActorClass->IsChildOf<AGameMode>() ||
		ActorClass->IsChildOf<ALODActor>() ||
		ActorClass->IsChildOf<ABrush>() ||
		ActorClass->IsChildOf<AHUD>())
	{
		return false;
	}

	// Is a child class of our non serialized classes?
	/*for (const auto& Class : IgnoredClasses)
	{
		if (ActorClass->IsChildOf(Class))
			return false;
	}*/

	return true;
}
