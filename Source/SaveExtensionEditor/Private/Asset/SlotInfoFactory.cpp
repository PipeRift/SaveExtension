// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "SlotInfoFactory.h"
#include "Kismet2/KismetEditorUtilities.h"


USlotInfoFactory::USlotInfoFactory() : Super() {
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USlotInfo::StaticClass();
}

UObject* USlotInfoFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) {
	check(Class->IsChildOf(USlotInfo::StaticClass()));

	if (!FKismetEditorUtilities::CanCreateBlueprintOfClass(Class))
	{
		return NULL;
	}
	return FKismetEditorUtilities::CreateBlueprint(Class,InParent,Name,BPTYPE_Const,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass(),TEXT("AssetTypeActions"));
}
