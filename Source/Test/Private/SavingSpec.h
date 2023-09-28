// Copyright 2015-2024 Piperift. All Rights Reserved.
#pragma once

#include "Helpers/TestActor.h"

#include <SaveSlot.h>

#include "SavingSpec.generated.h"


UCLASS()
class UTestSaveSlot_SyncSaving : public USaveSlot
{
	GENERATED_BODY()

	UTestSaveSlot_SyncSaving() : Super()
	{
		bStoreGameInstance = true;

		MultithreadedFiles = ESEAsyncMode::SaveAndLoadSync;
		MultithreadedSerialization = ESEAsyncMode::SaveAndLoadSync;
		ActorFilter.AllowedClasses.Add(ATestActor::StaticClass());
	}
};
