// Copyright 2015-2024 Piperift. All Rights Reserved.
#pragma once

#include "Helpers/TestGameInstance.h"

#include <SaveSlot.h>

#include "GameInstanceSpec.generated.h"


UCLASS()
class UTestSaveSlot : public USaveSlot
{
	GENERATED_BODY()

	UTestSaveSlot() : Super()
	{
		bStoreGameInstance = true;

		MultithreadedFiles = ESaveASyncMode::OnlySync;
		MultithreadedSerialization = ESaveASyncMode::OnlySync;
	}
};
