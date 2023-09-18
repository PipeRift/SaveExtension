// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "LevelFilter.h"

#include "SaveSlot.h"


/////////////////////////////////////////////////////
// USaveDataTask

const FName FSELevelFilter::TagNoTransform{"!SaveTransform"};
const FName FSELevelFilter::TagNoPhysics{"!SavePhysics"};
const FName FSELevelFilter::TagNoTags{"!SaveTags"};
const FName FSELevelFilter::TagTransform{"SaveTransform"};

void FSELevelFilter::FromSlot(const USaveSlot& Slot)
{
	ActorFilter = Slot.GetActorFilter(true);
	LoadActorFilter = Slot.GetActorFilter(false);
	bStoreComponents = Slot.bStoreComponents;
	ComponentFilter = Slot.GetComponentFilter(true);
	LoadComponentFilter = Slot.GetComponentFilter(false);
}

void FSELevelFilter::BakeAllowedClasses() const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSELevelFilter::BakeAllowedClasses);
	ActorFilter.BakeAllowedClasses();
	ComponentFilter.BakeAllowedClasses();
	LoadActorFilter.BakeAllowedClasses();
	LoadComponentFilter.BakeAllowedClasses();
}

bool FSELevelFilter::ShouldSave(const AActor* Actor) const
{
	return ActorFilter.IsClassAllowed(Actor->GetClass());
}

bool FSELevelFilter::ShouldSave(const UActorComponent* Component) const
{
	return IsValid(Component) && ComponentFilter.IsClassAllowed(Component->GetClass());
}

bool FSELevelFilter::ShouldLoad(const AActor* Actor) const
{
	return LoadActorFilter.IsClassAllowed(Actor->GetClass());
}

bool FSELevelFilter::ShouldLoad(const UActorComponent* Component) const
{
	return IsValid(Component) && LoadComponentFilter.IsClassAllowed(Component->GetClass());
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
