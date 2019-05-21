// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "SavePipeline.h"
#include "SaveManager.h"

#include "SlotInfo.h"
#include "SlotData.h"


void USavePipeline::EventBeginPlay_Implementation()
{
	BeginPlay();
}

void USavePipeline::EventSerializeSlot_Implementation()
{
	SerializeSlot();
}

void USavePipeline::EventTick_Implementation(float DeltaTime)
{
	Tick(DeltaTime);
}

void USavePipeline::EventEndPlay_Implementation()
{
	EndPlay();
}

void USavePipeline::BeginPlay()
{
	//AutoLoad
	if (Settings.bAutoLoad)
	{
		GetManager()->ReloadCurrentSlot();
	}
}

void USavePipeline::EndPlay()
{
	if (Settings.bSaveOnExit)
	{
		GetManager()->SaveCurrentSlot();
	}
}

UWorld* USavePipeline::GetWorld() const
{
	// If we are a CDO, we must return nullptr instead of calling Outer->GetWorld() to fool UObject::ImplementsGetWorld.
	if (HasAllFlags(RF_ClassDefaultObject))
		return nullptr;

	return GetOuter()->GetWorld();
}

class USaveManager* USavePipeline::GetManager() const
{
	return Cast<USaveManager>(GetOuter());
}
