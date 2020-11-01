// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "SavePresetFactory.h"
#include "Kismet2/KismetEditorUtilities.h"


USavePresetFactory::USavePresetFactory() : Super() {
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USavePreset::StaticClass();
}

UObject* USavePresetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) {
	check(Class->IsChildOf(USavePreset::StaticClass()));

	if (!FKismetEditorUtilities::CanCreateBlueprintOfClass(Class))
	{
		return nullptr;
	}
	return FKismetEditorUtilities::CreateBlueprint(Class, InParent, Name, BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(),TEXT("AssetTypeActions"));
}
