// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "AssetTypeActions_Base.h"
#include "Factories/Factory.h"
#include "SaveSlotData.h"

#include "SaveSlotDataFactory.generated.h"


UCLASS()
class SAVEEXTENSIONEDITOR_API USaveSaveSlotDataFactory : public UFactory
{
	GENERATED_BODY()

public:
	USaveSaveSlotDataFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
		UObject* Context, FFeedbackContext* Warn) override;
};
