// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "SavePresetFactory.h"
#include "Kismet2/KismetEditorUtilities.h"


USavePresetFactory::USavePresetFactory() : Super() {
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UDEPRECATED_SavePreset::StaticClass();
}

UObject* USavePresetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	// if we have no asset class, use the passed-in class instead
	check(Class == UDEPRECATED_SavePreset::StaticClass() || Class->IsChildOf(UDEPRECATED_SavePreset::StaticClass()));
	return NewObject<UDEPRECATED_SavePreset>(InParent, Class, Name, Flags);
}
