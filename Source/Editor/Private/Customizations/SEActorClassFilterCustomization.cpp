// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Customizations/SEActorClassFilterCustomization.h"

#include <DetailWidgetRow.h>
#include <Modules/ModuleManager.h>

#define LOCTEXT_NAMESPACE "FSEActorClassFilterCustomization"


TSharedPtr<IPropertyHandle> FSEActorClassFilterCustomization::GetFilterHandle(
	TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	return StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSEActorClassFilter, ClassFilter));
	;
}

#undef LOCTEXT_NAMESPACE
