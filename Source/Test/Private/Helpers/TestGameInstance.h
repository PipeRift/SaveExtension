
// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Engine/GameInstance.h>
#include "TestGameInstance.generated.h"

UCLASS()
class UTestGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:

    UPROPERTY(SaveGame)
    bool bMyBool = false;
};
