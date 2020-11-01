// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "SClassFilterGraphPin.h"
#include "Widgets/Input/SComboButton.h"
#include "GameplayTagsModule.h"
#include "Widgets/Layout/SScaleBox.h"

#define LOCTEXT_NAMESPACE "GameplayTagGraphPin"

void SClassFilterGraphPin::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	SGraphPin::Construct( SGraphPin::FArguments(), InGraphPinObj );
}

TSharedRef<SWidget>	SClassFilterGraphPin::GetDefaultValueWidget()
{
	ParseDefaultValueData();

	//Create widget
	return SNew(SHorizontalBox)
	+SHorizontalBox::Slot()
	.AutoWidth()
	.VAlign(VAlign_Fill)
	[
		SAssignNew( ComboButton, SComboButton )
		.OnGetMenuContent(this, &SClassFilterGraphPin::GetListContent)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton")
		.ForegroundColor(FSlateColor::UseForeground())
		.ContentPadding(FMargin(0.0f, 2.0f))
		.MenuPlacement(MenuPlacement_BelowAnchor)
		.Visibility( this, &SGraphPin::GetDefaultValueVisibility )
	]
	+SHorizontalBox::Slot()
	.AutoWidth()
	[
		SelectedTags()
	];
}

void SClassFilterGraphPin::ParseDefaultValueData()
{
	Filter.FromString(GraphPinObj->GetDefaultAsString());
}

TSharedRef<SWidget> SClassFilterGraphPin::GetListContent()
{
	EditableFilters.Empty();
	EditableFilters.Add( SClassFilter::FEditableClassFilterDatum( GraphPinObj->GetOwningNode(), &Filter ) );

	return SNew( SVerticalBox )
	+SVerticalBox::Slot()
	.AutoHeight()
	.MaxHeight( 400 )
	[
		SNew( SClassFilter, EditableFilters )
		.OnFilterChanged(this, &SClassFilterGraphPin::RefreshPreviewList)
		.Visibility( this, &SGraphPin::GetDefaultValueVisibility )
	];
}

TSharedRef<SWidget> SClassFilterGraphPin::SelectedTags()
{
	RefreshPreviewList();

	SAssignNew( PreviewList, SListView<TSharedPtr<FSEClassFilterItem>> )
		.ListItemsSource(&PreviewClasses)
		.SelectionMode(ESelectionMode::None)
		.OnGenerateRow(this, &SClassFilterGraphPin::OnGeneratePreviewRow);

	return PreviewList->AsShared();
}

TSharedRef<ITableRow> SClassFilterGraphPin::OnGeneratePreviewRow(TSharedPtr<FSEClassFilterItem> Class, const TSharedRef<STableViewBase>& OwnerTable)
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

void SClassFilterGraphPin::RefreshPreviewList()
{
	// Clear the list
	PreviewClasses.Empty();

	// Add classes to preview
	for (const auto& Class : Filter.AllowedClasses)
	{
		PreviewClasses.Add(MakeShared<FSEClassFilterItem>(Class.GetAssetName(), true));
	}
	for (const auto& Class : Filter.IgnoredClasses)
	{
		PreviewClasses.Add(MakeShared<FSEClassFilterItem>(Class.GetAssetName(), false));
	}

	// Set Pin Data
	FString ClassFilterString = Filter.ToString();
	FString CurrentDefaultValue = GraphPinObj->GetDefaultAsString();
	if (CurrentDefaultValue.IsEmpty())
	{
		CurrentDefaultValue = FString(TEXT("()"));
	}

	if (!CurrentDefaultValue.Equals(ClassFilterString))
	{
		GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, ClassFilterString);
	}

	// Refresh the slate list
	if (PreviewList.IsValid())
	{
		PreviewList->RequestListRefresh();
	}
}

#undef LOCTEXT_NAMESPACE
