// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "Customizations/ActorFilterCustomization.h"

#include <DetailWidgetRow.h>
#include <Widgets/Input/SComboButton.h>

#define LOCTEXT_NAMESPACE "FActorFilterCustomization"


void FActorFilterCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	/*HeaderRow
	.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MaxDesiredWidth(512)
	[
		SAssignNew(EditButton, SComboButton)
		.OnGetMenuContent(this, &FActorFilterCustomization::GetListContent)
		.OnMenuOpenChanged(this, &FActorFilterCustomization::OnGameplayTagListMenuOpenStateChanged)
		.ContentPadding(FMargin(2.0f, 2.0f))
		.MenuPlacement(MenuPlacement_BelowAnchor)
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("GameplayTagCustomization_Edit", "Edit"))
		]
	];*/
}

void FActorFilterCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{

}

TSharedPtr<SWidget> FActorFilterCustomization::GetListContent()
{
	if (!StructHandle.IsValid() || StructHandle->GetProperty() == nullptr)
	{
		return SNullWidget::NullWidget;
	}
	return SNullWidget::NullWidget;
}

#undef LOCTEXT_NAMESPACE