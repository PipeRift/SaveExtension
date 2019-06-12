// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "SaveGraph.h"
#include "SaveManager.h"

#include "SlotInfo.h"
#include "SlotData.h"


bool USaveGraph::EventPrepare_Implementation()
{
	return Prepare();
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
