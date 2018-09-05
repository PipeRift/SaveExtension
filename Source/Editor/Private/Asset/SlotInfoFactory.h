// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "SlotInfo.h"

#include "Public/AssetTypeActions_Base.h"
#include "Editor/UnrealEd/Classes/Factories/Factory.h"

#include "SlotInfoFactory.generated.h"


UCLASS()
class SAVEEXTENSIONEDITOR_API USlotInfoFactory : public UFactory {
	GENERATED_UCLASS_BODY()

protected:

	virtual bool IsMacroFactory() const { return false; }

public:

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
