// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "SavePresetFactory.h"
#include "Kismet2/KismetEditorUtilities.h"


USavePresetFactory::USavePresetFactory() : Super() {
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USavePreset::StaticClass();
}

UObject* USavePresetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	// if we have no asset class, use the passed-in class instead
	check(Class == USavePreset::StaticClass() || Class->IsChildOf(USavePreset::StaticClass()));
	return NewObject<USavePreset>(InParent, Class, Name, Flags);
}
