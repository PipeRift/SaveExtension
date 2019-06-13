// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "Customizations/ClassFilterCustomization.h"

#include <DetailWidgetRow.h>
#include <Widgets/Input/SComboButton.h>

#define LOCTEXT_NAMESPACE "FClassFilterCustomization"


void FClassFilterCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructHandle = StructPropertyHandle;

	HeaderRow
	.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MaxDesiredWidth(512)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SAssignNew(EditButton, SComboButton)
			.OnGetMenuContent(this, &FClassFilterCustomization::GetListContent)
			.OnMenuOpenChanged(this, &FClassFilterCustomization::OnPopupStateChanged)
			.ContentPadding(FMargin(2.0f, 2.0f))
			.MenuPlacement(MenuPlacement_BelowAnchor)
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ClassFilterCustomization_Edit", "Edit"))
			]
		]
	];
}

void FClassFilterCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{}

void FClassFilterCustomization::RefreshClassesList()
{
	// Rebuild Editable Filters as container references can become unsafe
	BuildEditableFilterList();
}

void FClassFilterCustomization::BuildEditableFilterList()
{
	EditableFilters.Empty();

	if (StructHandle.IsValid())
	{
		TArray<void*> RawStructData;
		StructHandle->AccessRawData(RawStructData);

		for (int32 ContainerIdx = 0; ContainerIdx < RawStructData.Num(); ++ContainerIdx)
		{
			EditableFilters.Add(SClassFilter::FEditableClassFilterDatum(nullptr, (FClassFilter*)RawStructData[ContainerIdx]));
		}
	}
}

TSharedRef<SWidget> FClassFilterCustomization::GetListContent()
{
	if (!StructHandle.IsValid() || StructHandle->GetProperty() == nullptr)
	{
		return SNullWidget::NullWidget;
	}

	bool bReadOnly = StructHandle->IsEditConst();

	TSharedRef<SClassFilter> EditPopup = SNew(SClassFilter, EditableFilters)
	.ReadOnly(bReadOnly)
	.OnFilterChanged(this, &FClassFilterCustomization::RefreshClassesList)
	.PropertyHandle(StructHandle);

	LastFilterPopup = EditPopup;

	return SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.AutoHeight()
	.MaxHeight(400.f)
	[
		EditPopup
	];
}

void FClassFilterCustomization::OnPopupStateChanged(bool bIsOpened)
{
	if (bIsOpened)
	{
		TSharedPtr<SClassFilter> EditFilterPopup = LastFilterPopup.Pin();
		if (EditFilterPopup.IsValid())
		{
			EditButton->SetMenuContentWidgetToFocus(EditFilterPopup->GetWidgetToFocusOnOpen());
		}
	}
}

#undef LOCTEXT_NAMESPACE