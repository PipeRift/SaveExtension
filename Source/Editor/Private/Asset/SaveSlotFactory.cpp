// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Asset/SaveSlotFactory.h"

#include <Kismet2/KismetEditorUtilities.h>


USaveSlotFactory::USaveSlotFactory() : Super()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USaveSlot::StaticClass();
}

UObject* USaveSlotFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn)
{
	check(Class->IsChildOf(USaveSlot::StaticClass()));

	if (!FKismetEditorUtilities::CanCreateBlueprintOfClass(Class))
	{
		return nullptr;
	}
	return FKismetEditorUtilities::CreateBlueprint(Class, InParent, Name, BPTYPE_Normal,
		UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), TEXT("AssetTypeActions"));
}
