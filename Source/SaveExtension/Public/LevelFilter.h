// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "ClassFilter.h"

#include <Components/ActorComponent.h>
#include <GameFramework/Actor.h>

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

public:
	UPROPERTY(SaveGame, BlueprintReadWrite)
	FSEClassFilter ActorFilter{AActor::StaticClass()};

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FSEClassFilter ComponentFilter{UActorComponent::StaticClass()};


	FSELevelFilter() = default;

	void BakeAllowedClasses() const;

	bool Stores(const AActor* Actor) const;
	bool StoresAnyComponents() const;
	bool Stores(const UActorComponent* Component) const;
};
