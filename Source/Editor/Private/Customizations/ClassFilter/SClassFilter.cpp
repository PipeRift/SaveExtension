// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "SClassFilter.h"
#include "Misc/ConfigCacheIni.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Images/SImage.h"
#include "EditorStyleSet.h"
#include "Widgets/SWindow.h"
#include "Dialogs/Dialogs.h"
#include "GameplayTagsModule.h"
#include "ScopedTransaction.h"
#include "Textures/SlateIcon.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Toolkits/AssetEditorManager.h"
#include "AssetToolsModule.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GameplayTagsSettings.h"
#include "Layout/WidgetPath.h"
#include "Framework/Application/SlateApplication.h"
#include "AssetData.h"
#include "Editor.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "GameplayTagWidget"

const FString SClassFilter::SettingsIniSection = TEXT("GameplayTagWidget");

void SClassFilter::Construct(const FArguments& InArgs, const TArray<FEditableClassFilterDatum>& EditableFilters)
{
	// If we're in management mode, we don't need to have editable tag containers.
	ensure(EditableFilters.Num() > 0);
	Filters = EditableFilters;

	OnFilterChanged = InArgs._OnFilterChanged;
	bReadOnly = InArgs._ReadOnly;
	bMultiSelect = InArgs._MultiSelect;
	PropertyHandle = InArgs._PropertyHandle;
	MaxHeight = InArgs._MaxHeight;

	// Tag the assets as transactional so they can support undo/redo
	TArray<UObject*> ObjectsToMarkTransactional;
	if (PropertyHandle.IsValid())
	{
		// If we have a property handle use that to find the objects that need to be transactional
		PropertyHandle->GetOuterObjects(ObjectsToMarkTransactional);
	}
	else
	{
		// Otherwise use the owner list
		for (int32 AssetIdx = 0; AssetIdx < Filters.Num(); ++AssetIdx)
		{
			ObjectsToMarkTransactional.Add(Filters[AssetIdx].Owner.Get());
		}
	}

	// Now actually mark the assembled objects
	for (UObject* ObjectToMark : ObjectsToMarkTransactional)
	{
		if (ObjectToMark)
		{
			ObjectToMark->SetFlags(RF_Transactional);
		}
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)

			// Gameplay Tag Tree controls
			+SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Top)
			[
				SNew(SHorizontalBox)

				// Search
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.FillWidth(1.f)
				.Padding(5, 1, 5, 1)
				[
					SAssignNew(SearchBox, SSearchBox)
					.HintText(LOCTEXT("ClassFilter_SearchBoxHint", "Search Classes"))
					.OnTextChanged(this, &SClassFilter::OnSearchTextChanged)
				]

				// Expand All nodes
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.OnClicked(this, &SClassFilter::OnClickedExpandAll)
					.Text(LOCTEXT("ClassFilter_ExpandAll", "Expand All"))
				]

				// Collapse All nodes
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.OnClicked(this, &SClassFilter::OnClickedCollapseAll)
					.Text(LOCTEXT("ClassFilter_CollapseAll", "Collapse All"))
				]

				// Clear selections
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.OnClicked(this, &SClassFilter::OnClickedClearAll)
					.Text(LOCTEXT("ClassFilter_ClearAll", "Clear All"))
					.Visibility(this, &SClassFilter::DetermineClearSelectionVisibility)
				]
			]

			// Classes tree
			+SVerticalBox::Slot()
			.MaxHeight(MaxHeight)
			[
				SAssignNew(TreeContainerWidget, SBorder)
				.Padding(FMargin(4.f))
				[
					SAssignNew(TreeWidget, STreeView<FClassFilterNodePtr>)
					.TreeItemsSource(&RootClasses)
					.OnGenerateRow(this, &SClassFilter::OnGenerateRow)
					.OnGetChildren(this, &SClassFilter::OnGetChildren)
					.OnExpansionChanged( this, &SClassFilter::OnExpansionChanged)
					.SelectionMode(ESelectionMode::Multi)
				]
			]
		]
	];

	// Force the entire tree collapsed to start
	SetTreeItemExpansion(false);
}

FVector2D SClassFilter::ComputeDesiredSize(float LayoutScaleMultiplier) const
	{
	FVector2D WidgetSize = SCompoundWidget::ComputeDesiredSize(LayoutScaleMultiplier);

	FVector2D TagTreeContainerSize = TreeContainerWidget->GetDesiredSize();

	if (TagTreeContainerSize.Y < MaxHeight)
	{
		WidgetSize.Y += MaxHeight - TagTreeContainerSize.Y;
	}

	return WidgetSize;
}

void SClassFilter::OnSearchTextChanged( const FText& SearchText )
{
	SearchString = SearchText.ToString();

	// #TODO: Apply filter
	TreeWidget->RequestTreeRefresh();
}

bool SClassFilter::FilterChildrenCheck(const FClassFilterNodePtr& Class)
{
	check(!Class);
	// Return true if checked

	for (const auto& ChildClass : Class->GetChildrenList())
	{
		if (FilterChildrenCheck(ChildClass))
		{
			return true;
		}
	}
	return false;
}

TSharedRef<ITableRow> SClassFilter::OnGenerateRow(FClassFilterNodePtr Class, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<FClassFilterNodePtr>, OwnerTable)
		.Style(FEditorStyle::Get(), "ClassFilterTreeView")
		[
			SNew( SHorizontalBox )

			// Tag Selection (selection mode only)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Left)
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(this, &SClassFilter::OnClassCheckChanged, Class)
				.IsChecked(this, &SClassFilter::IsClassChecked, Class)
				.ToolTipText(Class->GetClassTooltip())
				.IsEnabled(this, &SClassFilter::CanSelectClasses)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Class->GetClassName(true)))
				]
			]
		];
}

void SClassFilter::OnGetChildren(FClassFilterNodePtr Class, TArray<FClassFilterNodePtr>& OutChildren)
{
	for(auto& ChildClass : Class->GetChildrenList())
	{
		if(FilterChildrenCheck(ChildClass))
		{
			OutChildren.Add(ChildClass);
		}
	}
}

void SClassFilter::OnClassCheckChanged(ECheckBoxState NewCheckState, FClassFilterNodePtr NodeChanged)
{
	if (NewCheckState == ECheckBoxState::Checked)
	{
		OnClassChecked(NodeChanged);
	}
	else if (NewCheckState == ECheckBoxState::Unchecked)
	{
		OnClassUnchecked(NodeChanged);
	}
}

void SClassFilter::OnClassChecked(const FClassFilterNodePtr& Class)
{
	FScopedTransaction Transaction( LOCTEXT("ClassFilter_AllowClass", "Allow Class") );
	if (Class)
	{
	}
}

void SClassFilter::OnClassUnchecked(const FClassFilterNodePtr& Class)
{
	FScopedTransaction Transaction( LOCTEXT("ClassFinder_DeniedClass", "Denied Class"));
	if (Class)
	{
	}
}

void SClassFilter::UncheckChildren(const FClassFilterNodePtr& Class, FClassFilter& Filter)
{
	check(Class);

	// Uncheck Children
	for (auto& Child : Class->GetChildrenList())
	{
		UncheckChildren(Child, Filter);
	}
}

ECheckBoxState SClassFilter::IsClassChecked(FClassFilterNodePtr Class) const
{
	int32 NumValidAssets = 0;
	int32 NumAssetsTagIsAppliedTo = 0;

	if (Class)
	{
	}

	if (NumAssetsTagIsAppliedTo == 0)
	{
		return ECheckBoxState::Unchecked;
	}
	else if (NumAssetsTagIsAppliedTo == NumValidAssets)
	{
		return ECheckBoxState::Checked;
	}
	else
	{
		return ECheckBoxState::Undetermined;
	}
}

FReply SClassFilter::OnClickedClearAll()
{
	FScopedTransaction Transaction( LOCTEXT("GameplayTagWidget_RemoveAllTags", "Remove All Gameplay Tags") );

	for (int32 ContainerIdx = 0; ContainerIdx < Filters.Num(); ++ContainerIdx)
	{
		UObject* OwnerObj = Filters[ContainerIdx].Owner.Get();
		FClassFilter* Container = Filters[ContainerIdx].Filter;

		if (Container)
		{
			FClassFilter EmptyFilter;
			SetFilter(Container, &EmptyFilter, OwnerObj);
		}
	}
	return FReply::Handled();
}

FSlateColor SClassFilter::GetClassTextColour(const FClassFilterNodePtr& Node) const
{
	static const FLinearColor DefaultTextColour = FLinearColor::White;
	return DefaultTextColour;
}

FReply SClassFilter::OnClickedExpandAll()
{
	SetTreeItemExpansion(true);
	return FReply::Handled();
}

FReply SClassFilter::OnClickedCollapseAll()
{
	SetTreeItemExpansion(false);
	return FReply::Handled();
}

void SClassFilter::SetTreeItemExpansion(bool bExpand)
{

}

void SClassFilter::SetFilter(FClassFilter* OriginalFilter, FClassFilter* EditedFilter, UObject* OwnerObj)
{
	if (PropertyHandle.IsValid() && bMultiSelect)
	{
		// Case for a tag container
		//PropertyHandle->SetValueFromFormattedString(EditedFilter->ToString());
	}
	else if (PropertyHandle.IsValid() && !bMultiSelect)
	{
		// Case for a single Tag
		//FString FormattedString = TEXT("(TagName=\"");
		//FormattedString += EditedFilter.First().GetTagName().ToString();
		//FormattedString += TEXT("\")");
		//PropertyHandle->SetValueFromFormattedString(FormattedString);
	}
	else
	{
		// Not sure if we should get here, means the property handle hasn't been setup which could be right or wrong.
		if (OwnerObj)
		{
			OwnerObj->PreEditChange(PropertyHandle.IsValid() ? PropertyHandle->GetProperty() : nullptr);
		}

		*OriginalFilter = *EditedFilter;

		if (OwnerObj)
		{
			OwnerObj->PostEditChange();
		}
	}

	if (!PropertyHandle.IsValid())
	{
		OnFilterChanged.ExecuteIfBound();
	}
}

EVisibility SClassFilter::DetermineClearSelectionVisibility() const
{
	return CanSelectClasses() ? EVisibility::Visible : EVisibility::Collapsed;
}

void SClassFilter::OnExpansionChanged(FClassFilterNodePtr Class, bool bIsExpanded)
{

}

bool SClassFilter::CanSelectClasses() const
{
	return !bReadOnly;
}

TSharedPtr<SWidget> SClassFilter::GetWidgetToFocusOnOpen()
{
	return SearchBox;
}

#undef LOCTEXT_NAMESPACE
