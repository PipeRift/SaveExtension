// Copyright 2015-2024 Piperift. All Rights Reserved.
#pragma once

#include "Helpers/SETestActor.h"

#include <SaveSlot.h>

#include "Saving.spec.generated.h"


UCLASS()
class USETestSaveSlot_SyncSaving : public USaveSlot
{
	GENERATED_BODY()

	USETestSaveSlot_SyncSaving() : Super()
	{
		bStoreGameInstance = true;

		MultithreadedFiles = ESEAsyncMode::SaveAndLoadSync;
		MultithreadedSerialization = ESEAsyncMode::SaveAndLoadSync;
		ActorFilter.AllowedClasses.Add(ASETestActor::StaticClass());
	}
};
