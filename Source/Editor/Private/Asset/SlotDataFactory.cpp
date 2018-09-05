// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "SlotDataFactory.h"
#include "Kismet2/KismetEditorUtilities.h"


USlotDataFactory::USlotDataFactory(const class FObjectInitializer& OBJ) : Super(OBJ) {
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USlotData::StaticClass();
}

UObject* USlotDataFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) {
	check(Class->IsChildOf(USlotData::StaticClass()));

	if (!FKismetEditorUtilities::CanCreateBlueprintOfClass(Class))
	{
		return NULL;
	}
	return FKismetEditorUtilities::CreateBlueprint(Class,InParent,Name,BPTYPE_Const,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass(),TEXT("AssetTypeActions"));
}
