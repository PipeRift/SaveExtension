// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "LevelFilter.h"

#include "SaveSlot.h"


/////////////////////////////////////////////////////
// USaveDataTask

const FName FSELevelFilter::TagNoTransform{"!SaveTransform"};
const FName FSELevelFilter::TagNoPhysics{"!SavePhysics"};
const FName FSELevelFilter::TagNoTags{"!SaveTags"};
const FName FSELevelFilter::TagTransform{"SaveTransform"};


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

bool FSELevelFilter::StoresAnyComponents() const
{
	return ComponentFilter.IsAnyAllowed();
}
bool FSELevelFilter::Stores(const UActorComponent* Component) const
{
	return ComponentFilter.IsAllowed(Component->GetClass());
}

bool FSELevelFilter::StoresTransform(const UActorComponent* Component)
{
	return Component->GetClass()->IsChildOf<USceneComponent>() && HasTag(Component, TagTransform);
}

bool FSELevelFilter::StoresTags(const UActorComponent* Component)
{
	return !HasTag(Component, TagNoTags);
}

bool FSELevelFilter::IsSaveTag(const FName& Tag)
{
	return Tag == TagNoTransform || Tag == TagNoPhysics || Tag == TagNoTags;
}

bool FSELevelFilter::StoresTransform(const AActor* Actor)
{
	return Actor->IsRootComponentMovable() && !HasTag(Actor, TagNoTransform);
}
bool FSELevelFilter::StoresPhysics(const AActor* Actor)
{
	return !HasTag(Actor, TagNoPhysics);
}
bool FSELevelFilter::StoresTags(const AActor* Actor)
{
	return !HasTag(Actor, TagNoTags);
}
bool FSELevelFilter::IsProcedural(const AActor* Actor)
{
	return Actor->HasAnyFlags(RF_WasLoaded | RF_LoadCompleted);
}

bool FSELevelFilter::HasTag(const AActor* Actor, const FName Tag)
{
	check(Actor);
	return Actor->ActorHasTag(Tag);
}

bool FSELevelFilter::HasTag(const UActorComponent* Component, const FName Tag)
{
	check(Component);
	return Component->ComponentHasTag(Tag);
}
