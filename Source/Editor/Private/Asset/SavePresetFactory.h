// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "AssetTypeActions_Base.h"
#include "Factories/Factory.h"
#include "SavePreset.h"

#include "SavePresetFactory.generated.h"



UCLASS()
class SAVEEXTENSIONEDITOR_API USavePresetFactory : public UFactory
{
	GENERATED_BODY()

public:
	USavePresetFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
		UObject* Context, FFeedbackContext* Warn) override;
};
