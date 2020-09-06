
// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include "TestActor.generated.h"


UCLASS()
class ATestActor : public AActor
{
    GENERATED_BODY()

public:

    UPROPERTY(SaveGame)
    bool bMyBool = false;
};
