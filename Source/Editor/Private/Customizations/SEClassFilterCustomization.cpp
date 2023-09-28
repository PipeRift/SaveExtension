// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Customizations/SEClassFilterCustomization.h"

#include <DetailWidgetRow.h>
#include <Editor.h>
#include <PropertyCustomizationHelpers.h>
#include <ScopedTransaction.h>
#include <Styling/AppStyle.h>
#include <Widgets/Images/SImage.h>
#include <Widgets/Input/SButton.h>
#include <Widgets/Input/SComboButton.h>
#include <Widgets/Layout/SScaleBox.h>


#define LOCTEXT_NAMESPACE "FSEClassFilterCustomization"


FSEClassFilterCustomization::~FSEClassFilterCustomization()
{
	GEditor->UnregisterForUndo(this);
}

void FSEClassFilterCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructHandle = StructPropertyHandle;

	BuildEditableFilterList();

	// clang-format off
	HeaderRow
	.NameContent()
	[
		StructHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.HAlign(HAlign_Fill)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Fill)
		.MaxWidth(200.f)
		[
			SAssignNew(EditButton, SComboButton)
			.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
			.OnGetMenuContent(this, &FSEClassFilterCustomization::GetListContent)
			.OnMenuOpenChanged(this, &FSEClassFilterCustomization::OnPopupStateChanged)
			.ContentPadding(FMargin(2.0f, 2.0f))
			.MenuPlacement(MenuPlacement_BelowAnchor)
			.ForegroundColor(FSlateColor::UseForeground())
			.ButtonContent()
			[
				SNew(SBorder)
				.Padding(4.0f)
				.Visibility(this, &FSEClassFilterCustomization::GetClassPreviewVisibility)
				[
					GetClassPreview()
				]
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Fill)
		[
			PropertyCustomizationHelpers::MakeEmptyButton(FSimpleDelegate::CreateSP(this, &FSEClassFilterCustomization::OnClearClicked), LOCTEXT("ClassFilter_ClearTooltip", "Clear all allowed and ignored classes"))
		]
	];
	// clang-format on

	GEditor->RegisterForUndo(this);
}

void FSEClassFilterCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle,
	class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{}

void FSEClassFilterCustomization::BuildEditableFilterList()
{
	EditableFilters.Empty();
	if (StructHandle.IsValid())
	{
		TArray<void*> RawStructData;
		StructHandle->AccessRawData(RawStructData);

		TArray<UObject*> Outers;
		StructHandle->GetOuterObjects(Outers);
		UObject* FirstOuter = Outers.Num() ? Outers[0] : nullptr;

		for (int32 ContainerIdx = 0; ContainerIdx < RawStructData.Num(); ++ContainerIdx)
		{
			EditableFilters.Add(SClassFilter::FEditableClassFilterDatum(
				FirstOuter, (FSEClassFilter*) RawStructData[ContainerIdx]));
		}
	}
}

TSharedRef<SWidget> FSEClassFilterCustomization::GetListContent()
{
	if (!StructHandle.IsValid() || StructHandle->GetProperty() == nullptr)
	{
		return SNullWidget::NullWidget;
	}

	bool bReadOnly = StructHandle->IsEditConst();

	// clang-format off
	TSharedRef<SClassFilter> EditPopup =
		SNew(SClassFilter, EditableFilters)
		.ReadOnly(bReadOnly)
		.OnFilterChanged(this, &FSEClassFilterCustomization::RefreshClassList)
		.PropertyHandle(StructHandle);
	LastFilterPopup = EditPopup;
	return SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.AutoHeight()
	.MaxHeight(400.f)
	[
		EditPopup
	];
	// clang-format on
}

void FSEClassFilterCustomization::OnPopupStateChanged(bool bIsOpened)
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

void FSEClassFilterCustomization::OnClearClicked()
{
	FScopedTransaction Transaction(LOCTEXT("ClassFilter_Filter", "Clear Filter"));
	StructHandle->NotifyPreChange();
	for (auto& Filter : EditableFilters)
	{
		*Filter.Filter = {};
	}
	StructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	RefreshClassList();
}

EVisibility FSEClassFilterCustomization::GetClassPreviewVisibility() const
{
	return EVisibility::Visible;
	/*const auto* Filter = EditableFilters[0].Filter;
	const bool bEmpty = Filter->AllowedClasses.Num() > 0 ||
		Filter->IgnoredClasses.Num() > 0;
	return bEmpty ? EVisibility::Visible : EVisibility::Collapsed;*/
}

TSharedRef<SWidget> FSEClassFilterCustomization::GetClassPreview()
{
	RefreshClassList();

	// clang-format off
	return SNew(SBox)
	.MinDesiredWidth(100.f)
	[
		SAssignNew(PreviewList, SListView<TSharedPtr<FSEClassFilterItem>>)
		.SelectionMode(ESelectionMode::None)
		.ListItemsSource(&PreviewClasses)
		.OnGenerateRow(this, &FSEClassFilterCustomization::OnGeneratePreviewRow)
	];
	// clang-format on
}

TSharedRef<ITableRow> FSEClassFilterCustomization::OnGeneratePreviewRow(
	TSharedPtr<FSEClassFilterItem> Class, const TSharedRef<STableViewBase>& OwnerTable)
{
	FLinearColor StateColor;
	FText StateText;
	if (Class->bAllowed)
	{
		StateColor = FLinearColor::Green;
		StateText = FText::FromString(FString(TEXT("\xf00c"))) /*fa-check*/;
	}
	else
	{
		StateColor = FLinearColor::Red;
		StateText = FText::FromString(FString(TEXT("\xf00d"))) /*fa-times*/;
	}

	// clang-format off
	return SNew(STableRow<TSharedPtr<FSEClassFilterItem>>, OwnerTable)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 2, 0)
		[
			SNew(STextBlock)
			.ColorAndOpacity(StateColor)
			.Font(FAppStyle::Get().GetFontStyle("FontAwesome.8"))
			.Text(StateText)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(FText::FromString(Class->ClassName))
		]
	];
	// clang-format on
}

void FSEClassFilterCustomization::PostUndo(bool bSuccess)
{
	if (bSuccess)
	{
		RefreshClassList();
	}
}

void FSEClassFilterCustomization::PostRedo(bool bSuccess)
{
	if (bSuccess)
	{
		RefreshClassList();
	}
}

void FSEClassFilterCustomization::RefreshClassList()
{
	// Rebuild Editable Containers as container references can become unsafe
	BuildEditableFilterList();

	// Clear the list
	PreviewClasses.Reset();

	// Add tags to list
	for (const auto& Filter : EditableFilters)
	{
		if (Filter.Filter)
		{
			for (const auto& Class : Filter.Filter->AllowedClasses)
			{
				PreviewClasses.Add(MakeShared<FSEClassFilterItem>(Class.GetAssetName(), true));
			}

			for (const auto& Class : Filter.Filter->IgnoredClasses)
			{
				PreviewClasses.Add(MakeShared<FSEClassFilterItem>(Class.GetAssetName(), false));
			}
		}
	}

	// Refresh the slate list
	if (PreviewList.IsValid())
	{
		PreviewList->RequestListRefresh();
	}
}


#undef LOCTEXT_NAMESPACE