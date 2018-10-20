// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "SlotData.h"

#include "Public/AssetTypeActions_Base.h"
#include "Editor/UnrealEd/Classes/Factories/Factory.h"

#include "SlotDataFactory.generated.h"


UCLASS()
class SAVEEXTENSIONEDITOR_API USlotDataFactory : public UFactory {
	GENERATED_BODY()

public:

	USlotDataFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
