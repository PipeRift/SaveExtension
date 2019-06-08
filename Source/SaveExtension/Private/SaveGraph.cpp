// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "SaveGraph.h"
#include "SaveManager.h"

#include "SlotInfo.h"
#include "SlotData.h"


void USaveGraph::EventBeginPlay_Implementation()
{
	BeginPlay();
}

void USaveGraph::EventSerializeSlot_Implementation()
{
	SerializeSlot();
}

void USaveGraph::EventTick_Implementation(float DeltaTime)
{
	Tick(DeltaTime);
}

void USaveGraph::EventEndPlay_Implementation()
{
	EndPlay();
}

void USaveGraph::BeginPlay()
{
	//AutoLoad
	if (Settings.bAutoLoad)
	{
		GetManager()->ReloadCurrentSlot();
	}
}

void USaveGraph::EndPlay()
{
	if (Settings.bSaveOnExit)
	{
		GetManager()->SaveCurrentSlot();
	}
}

UWorld* USaveGraph::GetWorld() const
{
	// If we are a CDO, we must return nullptr instead of calling Outer->GetWorld() to fool UObject::ImplementsGetWorld.
	if (HasAllFlags(RF_ClassDefaultObject))
		return nullptr;

	return GetOuter()->GetWorld();
}

class USaveManager* USaveGraph::GetManager() const
{
	return Cast<USaveManager>(GetOuter());
}
