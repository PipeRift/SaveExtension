// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "LevelFilter.h"

#include "SaveSlot.h"


/////////////////////////////////////////////////////
// USaveDataTask


void FSELevelFilter::BakeAllowedClasses() const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSELevelFilter::BakeAllowedClasses);
	ActorFilter.BakeAllowedClasses();
	ComponentFilter.BakeAllowedClasses();
}

bool FSELevelFilter::Stores(const AActor* Actor) const
{
	return ActorFilter.IsAllowed(Actor->GetClass());
}

bool FSELevelFilter::Stores(const UActorComponent* Component) const
{
	return ComponentFilter.IsAllowed(Component->GetClass());
}
