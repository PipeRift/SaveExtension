// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "SlotInfo.h"

#include "AssetTypeActions_Base.h"
#include "Factories/Factory.h"

#include "SlotInfoFactory.generated.h"


UCLASS()
class SAVEEXTENSIONEDITOR_API USlotInfoFactory : public UFactory {
	GENERATED_BODY()

public:

	USlotInfoFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
