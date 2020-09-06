// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "Customizations/ComponentClassFilterCustomization.h"
#include "PropertyHandle.h"

#define LOCTEXT_NAMESPACE "FComponentClassFilterCustomization"


TSharedPtr<IPropertyHandle> FComponentClassFilterCustomization::GetFilterHandle(TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	return StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FComponentClassFilter, ClassFilter));;
}

#undef LOCTEXT_NAMESPACE
