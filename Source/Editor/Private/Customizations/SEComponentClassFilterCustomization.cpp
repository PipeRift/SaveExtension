// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Customizations/SEComponentClassFilterCustomization.h"

#include "PropertyHandle.h"


#define LOCTEXT_NAMESPACE "FSEComponentClassFilterCustomization"


TSharedPtr<IPropertyHandle> FSEComponentClassFilterCustomization::GetFilterHandle(
	TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	return StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSEComponentClassFilter, ClassFilter));
	;
}

#undef LOCTEXT_NAMESPACE
