// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Asset/SaveSlotDataFactory.h"

#include <Kismet2/KismetEditorUtilities.h>


USaveSaveSlotDataFactory::USaveSaveSlotDataFactory() : Super()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USaveSlotData::StaticClass();
}

UObject* USaveSaveSlotDataFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	check(Class->IsChildOf(USaveSlotData::StaticClass()));

	if (!FKismetEditorUtilities::CanCreateBlueprintOfClass(Class))
	{
		return nullptr;
	}
	return FKismetEditorUtilities::CreateBlueprint(Class, InParent, Name, BPTYPE_Normal,
		UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), TEXT("AssetTypeActions"));
}
