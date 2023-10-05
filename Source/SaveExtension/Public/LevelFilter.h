// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "ClassFilter.h"

#include "LevelFilter.generated.h"


class USaveManager;
class USaveSlot;


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
	FSEClassFilter ActorFilter;

	UPROPERTY(SaveGame)
	FSEClassFilter ComponentFilter;


	FSELevelFilter() = default;

	void BakeAllowedClasses() const;

	bool Stores(const AActor* Actor) const;
	bool StoresAnyComponents() const;
	bool Stores(const UActorComponent* Component) const;

	static bool StoresTransform(const UActorComponent* Component);
	static bool StoresTags(const UActorComponent* Component);

	static bool IsSaveTag(const FName& Tag);

	static bool StoresTransform(const AActor* Actor);
	static bool StoresPhysics(const AActor* Actor);
	static bool StoresTags(const AActor* Actor);
	static bool IsProcedural(const AActor* Actor);

	static bool HasTag(const AActor* Actor, const FName Tag);
	static bool HasTag(const UActorComponent* Component, const FName Tag);
};
