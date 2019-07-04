// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "Customizations/ClassFilterCustomization.h"

#include <DetailWidgetRow.h>
#include <Widgets/Input/SComboButton.h>
#include <Widgets/Input/SButton.h>
#include <ScopedTransaction.h>
#include <Editor.h>
#include <Widgets/Images/SImage.h>
#include <Widgets/Layout/SScaleBox.h>

#define LOCTEXT_NAMESPACE "FClassFilterCustomization"


FClassFilterCustomization::~FClassFilterCustomization()
{
	GEditor->UnregisterForUndo(this);
}

void FClassFilterCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructHandle = StructPropertyHandle;
	FilterHandle = GetFilterHandle(StructPropertyHandle);

	BuildEditableFilterList();

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
			.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
			.OnGetMenuContent(this, &FClassFilterCustomization::GetListContent)
			.OnMenuOpenChanged(this, &FClassFilterCustomization::OnPopupStateChanged)
			.ContentPadding(FMargin(2.0f, 2.0f))
			.MenuPlacement(MenuPlacement_BelowAnchor)
			.ForegroundColor(FSlateColor::UseForeground())
			.ButtonContent()
			[
				SNew(SBorder)
				.Padding(4.0f)
				.Visibility(this, &FClassFilterCustomization::GetClassPreviewVisibility)
				[
					GetClassPreview()
				]
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Fill)
		[
			SNew(SButton)
			.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
			.ContentPadding(FMargin(2.0f, 2.0f))
			.OnClicked(this, &FClassFilterCustomization::OnClearClicked)
			.ForegroundColor(FSlateColor::UseForeground())
			.Text(LOCTEXT("ClassFilter_Clear", "Clear"))
			.ToolTipText(LOCTEXT("ClassFilter_ClearTooltip", "Clear all allowed and ignored classes"))
			[
				SNew(SScaleBox)
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("PropertyWindow.Button_EmptyArray"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
		]
	];

	GEditor->RegisterForUndo(this);
}

void FClassFilterCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{}

void FClassFilterCustomization::BuildEditableFilterList()
{
	EditableFilters.Empty();
	if (FilterHandle.IsValid())
	{
		TArray<void*> RawStructData;
		FilterHandle->AccessRawData(RawStructData);

		TArray<UObject*> Outers;
		FilterHandle->GetOuterObjects(Outers);
		UObject* FirstOuter = Outers.Num() ? Outers[0] : nullptr;

		for (int32 ContainerIdx = 0; ContainerIdx < RawStructData.Num(); ++ContainerIdx)
		{
			EditableFilters.Add(SClassFilter::FEditableClassFilterDatum(FirstOuter, (FClassFilter*)RawStructData[ContainerIdx]));
		}
	}
}

TSharedRef<SWidget> FClassFilterCustomization::GetListContent()
{
	if (!FilterHandle.IsValid() || FilterHandle->GetProperty() == nullptr)
	{
		return SNullWidget::NullWidget;
	}

	bool bReadOnly = FilterHandle->IsEditConst();

	TSharedRef<SClassFilter> EditPopup = SNew(SClassFilter, EditableFilters)
	.ReadOnly(bReadOnly)
	.OnFilterChanged(this, &FClassFilterCustomization::RefreshClassList)
	.PropertyHandle(FilterHandle);

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

FReply FClassFilterCustomization::OnClearClicked()
{
	FScopedTransaction Transaction(LOCTEXT("ClassFilter_Filter", "Clear Filter"));
	for (auto& Filter : EditableFilters)
	{
		// Reset Filter
		if (Filter.Owner.IsValid())
		{
			Filter.Owner->Modify();
		}

		*Filter.Filter = {};
	}

	RefreshClassList();
	return FReply::Handled();
}

EVisibility FClassFilterCustomization::GetClassPreviewVisibility() const
{
	return EVisibility::Visible;
	/*const auto* Filter = EditableFilters[0].Filter;
	const bool bEmpty = Filter->AllowedClasses.Num() > 0 ||
		Filter->IgnoredClasses.Num() > 0;
	return bEmpty ? EVisibility::Visible : EVisibility::Collapsed;*/
}

TSharedRef<SWidget> FClassFilterCustomization::GetClassPreview()
{
	RefreshClassList();

	return SNew(SBox)
	.MinDesiredWidth(100.f)
	[
		SAssignNew(PreviewList, SListView<TSharedPtr<FClassFilterItem>>)
		.SelectionMode(ESelectionMode::None)
		.ListItemsSource(&PreviewClasses)
		.OnGenerateRow(this, &FClassFilterCustomization::OnGeneratePreviewRow)
	];
}

TSharedRef<ITableRow> FClassFilterCustomization::OnGeneratePreviewRow(TSharedPtr<FClassFilterItem> Class, const TSharedRef<STableViewBase>& OwnerTable)
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

	return SNew(STableRow<TSharedPtr<FClassFilterItem>>, OwnerTable)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 2, 0)
		[
			SNew(STextBlock)
			.ColorAndOpacity(StateColor)
			.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.8"))
			.Text(StateText)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(FText::FromString(Class->ClassName))
		]
	];
}

void FClassFilterCustomization::PostUndo(bool bSuccess)
{
	if (bSuccess)
	{
		RefreshClassList();
	}
}

void FClassFilterCustomization::PostRedo(bool bSuccess)
{
	if (bSuccess)
	{
		RefreshClassList();
	}
}

void FClassFilterCustomization::RefreshClassList()
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
				PreviewClasses.Add(MakeShared<FClassFilterItem>(Class.GetAssetName(), true));
			}

			for (const auto& Class : Filter.Filter->IgnoredClasses)
			{
				PreviewClasses.Add(MakeShared<FClassFilterItem>(Class.GetAssetName(), false));
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