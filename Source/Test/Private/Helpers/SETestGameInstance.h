
// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Engine/GameInstance.h>

#include "SETestGameInstance.generated.h"


UCLASS()
class USETestGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	bool bMyBool = false;
};
