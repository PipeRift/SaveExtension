// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "SavePreset.h"
#include "SaveFilter.generated.h"

class USaveManager;


/**
 * Contains all settings that affect saving.
 * This information is saved to be restored while loading.
 */
USTRUCT(Blueprintable)
struct FSaveFilter
{
	GENERATED_BODY()

	static const FName TagNoTransform;
	static const FName TagNoPhysics;
	static const FName TagNoTags;
	static const FName TagTransform;

public:

	UPROPERTY(SaveGame)
	FActorClassFilter ActorFilter;

	UPROPERTY(SaveGame)
	FActorClassFilter LoadActorFilter;

	UPROPERTY(SaveGame)
	bool bStoreComponents = false;

	UPROPERTY(SaveGame)
	FComponentClassFilter ComponentFilter;

	UPROPERTY(SaveGame)
	FComponentClassFilter LoadComponentFilter;

	UPROPERTY(SaveGame)
	float MaxFrameMs = 5.f;

	UPROPERTY(SaveGame)
	bool bStoreGameInstance = false;

	UPROPERTY(SaveGame)
	bool bStoreLevelBlueprints = false;

	UPROPERTY(SaveGame)
	bool bStoreControlRotation = true;


	FSaveFilter() {}
	FSaveFilter(const USavePreset& Preset)
	{
		ActorFilter         = Preset.GetActorFilter(true);
		ComponentFilter     = Preset.GetComponentFilter(true);
		LoadActorFilter     = Preset.GetActorFilter(false);
		LoadComponentFilter = Preset.GetComponentFilter(false);
		BakeAllowedClasses();
		MaxFrameMs       = Preset.GetMaxFrameMs();
		bStoreComponents = Preset.bStoreComponents;
		bStoreGameInstance = Preset.bStoreGameInstance;
	}

	void BakeAllowedClasses()
	{
		ActorFilter.BakeAllowedClasses();
		ComponentFilter.BakeAllowedClasses();
		LoadActorFilter.BakeAllowedClasses();
		LoadComponentFilter.BakeAllowedClasses();
	}

	FORCEINLINE bool ShouldSave(const AActor* Actor) const
	{
		return ActorFilter.IsClassAllowed(Actor->GetClass());
	}

	FORCEINLINE bool ShouldLoad(const AActor* Actor) const
	{
		return LoadActorFilter.IsClassAllowed(Actor->GetClass());
	}

	FORCEINLINE bool ShouldSave(const UActorComponent* Component) const
	{
		return IsValid(Component)
			&& ComponentFilter.IsClassAllowed(Component->GetClass());
	}

	FORCEINLINE bool ShouldLoad(const UActorComponent* Component) const
	{
		return IsValid(Component)
			&& LoadComponentFilter.IsClassAllowed(Component->GetClass());
	}

	FORCEINLINE bool StoresTransform(const UActorComponent* Component) const
	{
		return Component->GetClass()->IsChildOf<USceneComponent>()
			&& HasTag(Component, TagTransform);
	}

	FORCEINLINE bool StoresTags(const UActorComponent* Component) const
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
