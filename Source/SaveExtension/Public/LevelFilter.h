// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include "SavePreset.h"
#include "LevelFilter.generated.h"

class USaveManager;


/**
 * Contains all settings that affect saving.
 * This information is saved to be restored while loading.
 */
USTRUCT(Blueprintable)
struct FSELevelFilter
{
	GENERATED_BODY()

	static const FName TagNoTransform;
	static const FName TagNoPhysics;
	static const FName TagNoTags;
	static const FName TagTransform;

public:

	UPROPERTY(SaveGame)
	FSEActorClassFilter ActorFilter;

	UPROPERTY(SaveGame)
	FSEActorClassFilter LoadActorFilter;

	UPROPERTY(SaveGame)
	bool bStoreComponents = false;

	UPROPERTY(SaveGame)
	FSEComponentClassFilter ComponentFilter;

	UPROPERTY(SaveGame)
	FSEComponentClassFilter LoadComponentFilter;


	FSELevelFilter() {}

	void FromPreset(const USavePreset& Preset)
	{
		ActorFilter = Preset.GetActorFilter(true);
		LoadActorFilter = Preset.GetActorFilter(false);
		bStoreComponents = Preset.bStoreComponents;
		ComponentFilter = Preset.GetComponentFilter(true);
		LoadComponentFilter = Preset.GetComponentFilter(false);
	}

	void BakeAllowedClasses() const
	{
		ActorFilter.BakeAllowedClasses();
		ComponentFilter.BakeAllowedClasses();
		LoadActorFilter.BakeAllowedClasses();
		LoadComponentFilter.BakeAllowedClasses();
	}

	bool ShouldSave(const AActor* Actor) const
	{
		return ActorFilter.IsClassAllowed(Actor->GetClass());
	}

	bool ShouldLoad(const AActor* Actor) const
	{
		return LoadActorFilter.IsClassAllowed(Actor->GetClass());
	}

	bool ShouldSave(const UActorComponent* Component) const
	{
		return IsValid(Component)
			&& ComponentFilter.IsClassAllowed(Component->GetClass());
	}

	bool ShouldLoad(const UActorComponent* Component) const
	{
		return IsValid(Component)
			&& LoadComponentFilter.IsClassAllowed(Component->GetClass());
	}

	static bool StoresTransform(const UActorComponent* Component)
	{
		return Component->GetClass()->IsChildOf<USceneComponent>()
			&& HasTag(Component, TagTransform);
	}

	static bool StoresTags(const UActorComponent* Component)
	{
		return !HasTag(Component, TagNoTags);
	}

	static bool IsSaveTag(const FName& Tag)
	{
		return Tag == TagNoTransform || Tag == TagNoPhysics || Tag == TagNoTags;
	}

	static FORCEINLINE bool StoresTransform(const AActor* Actor) { return Actor->IsRootComponentMovable() && !HasTag(Actor, TagNoTransform); }
	static FORCEINLINE bool StoresPhysics(const AActor* Actor)   { return !HasTag(Actor, TagNoPhysics); }
	static FORCEINLINE bool StoresTags(const AActor* Actor)      { return !HasTag(Actor, TagNoTags); }
	static FORCEINLINE bool IsProcedural(const AActor* Actor)    { return Actor->HasAnyFlags(RF_WasLoaded | RF_LoadCompleted); }

	static FORCEINLINE bool HasTag(const AActor* Actor, const FName Tag)
	{
		check(Actor);
		return Actor->ActorHasTag(Tag);
	}

	static FORCEINLINE bool HasTag(const UActorComponent * Component, const FName Tag)
	{
		check(Component);
		return Component->ComponentHasTag(Tag);
	}
};
