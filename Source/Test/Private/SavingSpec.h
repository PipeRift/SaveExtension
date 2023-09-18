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

		MultithreadedFiles = ESaveASyncMode::OnlySync;
		MultithreadedSerialization = ESaveASyncMode::OnlySync;
		ActorFilter.ClassFilter.AllowedClasses.Add(ATestActor::StaticClass());
	}
};
