// Copyright 2015-2024 Piperift. All Rights Reserved.
#pragma once

#include "Helpers/SETestGameInstance.h"

#include <SaveSlot.h>

#include "GameInstanceSpec.generated.h"


UCLASS()
class UTestSaveSlot : public USaveSlot
{
	GENERATED_BODY()

	UTestSaveSlot() : Super()
	{
		bStoreGameInstance = true;

		MultithreadedFiles = ESEAsyncMode::SaveAndLoadSync;
		MultithreadedSerialization = ESEAsyncMode::SaveAndLoadSync;
	}
};
