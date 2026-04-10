// Copyright 2015-2024 Piperift. All Rights Reserved.
#pragma once

#include "Helpers/SETestGameInstance.h"

#include <SaveSlot.h>

#include "GameInstance.spec.generated.h"


UCLASS()
class USETestSaveSlot : public USaveSlot
{
	GENERATED_BODY()

	USETestSaveSlot() : Super()
	{
		bStoreGameInstance = true;

		MultithreadedFiles = ESEAsyncMode::SaveAndLoadSync;
		MultithreadedSerialization = ESEAsyncMode::SaveAndLoadSync;
	}
};
