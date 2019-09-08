// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "Customizations/ActorClassFilterCustomization.h"

#include <DetailWidgetRow.h>
#include <Modules/ModuleManager.h>

#define LOCTEXT_NAMESPACE "FActorClassFilterCustomization"


TSharedPtr<IPropertyHandle> FActorClassFilterCustomization::GetFilterHandle(TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	return StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FActorClassFilter, ClassFilter));;
}

#undef LOCTEXT_NAMESPACE
