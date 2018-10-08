// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "SlotDataTask.h"

#include <Engine/StaticMeshActor.h>
#include <Engine/ReflectionCapture.h>
#include <Engine/LODActor.h>
#include <Lightmass/LightmassPortal.h>
#include <GameFramework/GameMode.h>
#include <GameFramework/GameState.h>
#include <GameFramework/PlayerState.h>
#include <GameFramework/PlayerController.h>
#include <InstancedFoliageActor.h>

#include "SaveManager.h"
#include "SavePreset.h"


/////////////////////////////////////////////////////
// USaveDataTask

const FName USlotDataTask::TagNoSave{ "!Save" };
const FName USlotDataTask::TagNoTransform{ "!SaveTransform" };
const FName USlotDataTask::TagNoPhysics{ "!SavePhysics" };
const FName USlotDataTask::TagNoComponents{ "!SaveComponents" };
const FName USlotDataTask::TagNoTags{ "!SaveTags" };
const FName USlotDataTask::TagTransform{ "SaveTransform" };


bool USlotDataTask::IsSaveTag(const FName& Tag)
{
	return Tag == TagNoSave || Tag == TagNoTransform || Tag == TagNoPhysics || Tag == TagNoComponents || Tag == TagNoTags;
}

USlotDataTask* USlotDataTask::Start()
{
	const USaveManager* Manager = GetManager();

	// If not running and first task is this
	if (!bRunning && Manager->Tasks.Num() > 0 && Manager->Tasks[0] == this)
	{
		bRunning = true;
		OnStart();
	}
	return this;
}

void USlotDataTask::Finish(bool bInSuccess)
{
	if (bRunning)
	{
		OnFinish();
		MarkPendingKill();
		GetManager()->FinishTask(this);
		bFinished = true;
		bSuccess = bInSuccess;
	}
}

bool USlotDataTask::IsScheduled() const
{
	return GetManager()->Tasks.Contains(this);
}

USaveManager* USlotDataTask::GetManager() const
{
	return Cast<USaveManager>(GetOuter());
}

UWorld* USlotDataTask::GetWorld() const
{
	return GetOuter()->GetWorld();
}

bool USlotDataTask::ShouldSaveAsWorld(const AActor* Actor) const
{
	if (ShouldSave(Actor))
	{
		const UClass* const ActorClass = Actor->GetClass();
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
		for (const auto& Class : Preset->IgnoredActors)
		{
			if (ActorClass->IsChildOf(Class))
				return false;
		}

		return true;
	}
	return false;
}
