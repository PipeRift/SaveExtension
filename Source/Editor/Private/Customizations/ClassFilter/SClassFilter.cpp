// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "SClassFilter.h"

#include "AssetRegistry/AssetData.h"
#include "ClassFilterHelpers.h"
#include "Dialogs/Dialogs.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GameplayTagsModule.h"
#include "GameplayTagsSettings.h"
#include "Layout/WidgetPath.h"
#include "Misc/ConfigCacheIni.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/SWindow.h"

#include <AssetToolsModule.h>
#include <ClassViewerProjectSettings.h>
#include <PropertyHandle.h>
#include <ScopedTransaction.h>
#include <Textures/SlateIcon.h>
#include <Widgets/Input/SHyperlink.h>
#include <Widgets/Input/SSearchBox.h>
#include <Widgets/Layout/SBorder.h>
#include <Widgets/Layout/SScaleBox.h>
#include <Widgets/Notifications/SNotificationList.h>


#define LOCTEXT_NAMESPACE "GameplayTagWidget"


void SClassFilter::Construct(
	const FArguments& InArgs, const TArray<FEditableClassFilterDatum>& EditableFilters)
{
	bNeedsRefresh = true;

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
		for (const auto& Filter : Filters)
		{
			ObjectsToMarkTransactional.Add(Filter.Owner.Get());
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
		[SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[SNew(SVerticalBox)

					// Gameplay Tag Tree controls
					+
					SVerticalBox::Slot().AutoHeight().VAlign(VAlign_Top)
						[SNew(SHorizontalBox)

							// Search
							+ SHorizontalBox::Slot()
								  .VAlign(VAlign_Center)
								  .FillWidth(1.f)
								  .Padding(5, 1, 5,
									  1)[SAssignNew(SearchBox, SSearchBox)
											 .HintText(LOCTEXT("ClassFilter_SearchBoxHint", "Search Classes"))
											 .OnTextChanged(this, &SClassFilter::OnSearchTextChanged)]

							// Expand All nodes
							+ SHorizontalBox::Slot()
								  .AutoWidth()[SNew(SButton)
												   .OnClicked(this, &SClassFilter::OnClickedExpandAll)
												   .Text(LOCTEXT("ClassFilter_ExpandAll", "Expand All"))]

							// Collapse All nodes
							+ SHorizontalBox::Slot()
								  .AutoWidth()[SNew(SButton)
												   .OnClicked(this, &SClassFilter::OnClickedCollapseAll)
												   .Text(LOCTEXT("ClassFilter_CollapseAll", "Collapse All"))]

							// Clear selections
							+ SHorizontalBox::Slot()
								  .AutoWidth()[SNew(SButton)
												   .OnClicked(this, &SClassFilter::OnClickedClearAll)
												   .Text(LOCTEXT("ClassFilter_ClearAll", "Clear All"))
												   .Visibility(this,
													   &SClassFilter::DetermineClearSelectionVisibility)]]

					// Classes tree
					+ SVerticalBox::Slot().MaxHeight(MaxHeight)
						  [SAssignNew(TreeContainerWidget, SBorder)
								  .Padding(FMargin(
									  4.f))[SAssignNew(TreeWidget, STreeView<FSEClassFilterNodePtr>)
												.TreeItemsSource(&RootClasses)
												.OnGenerateRow(this, &SClassFilter::OnGenerateRow)
												.OnGetChildren(this, &SClassFilter::OnGetChildren)
												.OnExpansionChanged(this, &SClassFilter::OnExpansionChanged)
												.SelectionMode(ESelectionMode::Multi)]]]];

	// Construct the class hierarchy.
	ClassFilter::Helpers::ConstructClassHierarchy();

	// Force the entire tree collapsed to start
	SetTreeItemExpansion(false);

	ClassFilter::Helpers::PopulateClassFilterDelegate.AddSP(this, &SClassFilter::Refresh);
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

void SClassFilter::OnSearchTextChanged(const FText& SearchText)
{
	SearchString = SearchText.ToString();

	if (SearchString.IsEmpty())
	{
		TreeWidget->SetTreeItemsSource(&RootClasses);

		for (const auto& Class : RootClasses)
		{
			SetDefaultTreeItemExpansion(Class);
		}
	}
	else
	{
		FilteredClasses.Reset();

		for (const auto& Class : RootClasses)
		{
			if (FilterChildrenCheck(Class))
			{
				FilteredClasses.Add(Class);
				SetTreeItemExpansion(Class, true);
			}
			else
			{
				SetTreeItemExpansion(Class, false);
			}
		}

		TreeWidget->SetTreeItemsSource(&FilteredClasses);
	}

	TreeWidget->RequestTreeRefresh();
}

bool SClassFilter::FilterChildrenCheck(const FSEClassFilterNodePtr& Class)
{
	check(Class.IsValid());

	// Return true if checked

	if (SearchString.IsEmpty())
	{
		return true;
	}

	if (Class->GetClassName().Contains(SearchString))
	{
		return true;
	}

	for (const auto& ChildClass : Class->GetChildrenList())
	{
		if (FilterChildrenCheck(ChildClass))
		{
			return true;
		}
	}
	return false;
}

TSharedRef<ITableRow> SClassFilter::OnGenerateRow(
	FSEClassFilterNodePtr Class, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<FSEClassFilterNodePtr>, OwnerTable)
		.Style(FAppStyle::Get(), "GameplayTagTreeView")
			[SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.BorderBackgroundColor(this, &SClassFilter::GetClassBackgroundColor, Class)
					.Padding(0)
					.Content()[SNew(SHorizontalBox) +
							   SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Left)
								   [SNew(SButton)
										   .ButtonStyle(FAppStyle::Get(), "FlatButton")
										   .OnClicked(this, &SClassFilter::OnClassClicked, Class)
										   .ForegroundColor(this, &SClassFilter::GetClassIconColor, Class)
										   .ContentPadding(0)
										   .IsEnabled(this, &SClassFilter::CanSelectClasses)
											   [SNew(STextBlock)
													   .Font(FAppStyle::Get().GetFontStyle("FontAwesome.8"))
													   .Text(this, &SClassFilter::GetClassIconText, Class)]]
							   // Tag Selection (selection mode only)
							   + SHorizontalBox::Slot().FillWidth(1.0f).HAlign(
									 HAlign_Left)[SNew(STextBlock)
													  .Text(FText::FromString(Class->GetClassName(true)))
													  .ToolTipText(Class->GetClassTooltip())
													  .IsEnabled(this, &SClassFilter::CanSelectClasses)]]];
}

void SClassFilter::OnGetChildren(FSEClassFilterNodePtr Class, TArray<FSEClassFilterNodePtr>& OutChildren)
{
	for (auto& ChildClass : Class->GetChildrenList())
	{
		if (FilterChildrenCheck(ChildClass))
		{
			OutChildren.Add(ChildClass);
		}
	}
}

FReply SClassFilter::OnClassClicked(FSEClassFilterNodePtr Class)
{
	const EClassFilterState OwnState = Class->GetOwnFilterState();
	if (OwnState == EClassFilterState::None)
	{
		const EClassFilterState ParentState = Class->GetParentFilterState();
		if (ParentState == EClassFilterState::Allowed)
		{
			MarkClass(Class, EClassFilterState::Denied);
		}
		else
		{
			MarkClass(Class, EClassFilterState::Allowed);
		}
	}
	else
	{
		MarkClass(Class, EClassFilterState::None);
	}
	return FReply::Handled();
}

FText SClassFilter::GetClassIconText(FSEClassFilterNodePtr Class) const
{
	switch (Class->GetOwnFilterState())
	{
		case EClassFilterState::Allowed:
			return FText::FromString(FString(TEXT("\xf00c"))) /*fa-check*/;

		case EClassFilterState::Denied:
			return FText::FromString(FString(TEXT("\xf00d"))) /*fa-times*/;
	}

	return FText::FromString(FString(TEXT("\xf096"))) /*fa-square-o*/;
}

FSlateColor SClassFilter::GetClassIconColor(FSEClassFilterNodePtr Class) const
{
	switch (Class->GetOwnFilterState())
	{
		case EClassFilterState::Allowed:
			return {FLinearColor::Green};

		case EClassFilterState::Denied:
			return {FLinearColor::Red};
	}

	return FSlateColor::UseForeground();
}

void SClassFilter::MarkClass(FSEClassFilterNodePtr Class, EClassFilterState State)
{
	const TSoftClassPtr<> ClassAsset{Class->ClassPath.ToString()};

	switch (State)
	{
		case EClassFilterState::Allowed: {
			FScopedTransaction Transaction(LOCTEXT("ClassFilter_AllowClass", "Allow Class"));
			if (Class)
			{
				Class->SetOwnFilterState(State);

				if (PropertyHandle)
					PropertyHandle->NotifyPreChange();
				for (const auto& Filter : Filters)
				{
					Filter.Filter->AllowedClasses.Add(ClassAsset);
					Filter.Filter->IgnoredClasses.Remove(ClassAsset);
				}
				if (PropertyHandle)
					PropertyHandle->NotifyPostChange(EPropertyChangeType::Unspecified);
			}
			break;
		}
		case EClassFilterState::Denied: {
			FScopedTransaction Transaction(LOCTEXT("ClassFilter_DeniedClass", "Deny Class"));
			if (Class)
			{
				Class->SetOwnFilterState(State);

				if (PropertyHandle)
					PropertyHandle->NotifyPreChange();
				for (const auto& Filter : Filters)
				{
					Filter.Filter->IgnoredClasses.Add(ClassAsset);
					Filter.Filter->AllowedClasses.Remove(ClassAsset);
				}
				if (PropertyHandle)
					PropertyHandle->NotifyPostChange(EPropertyChangeType::Unspecified);
			}
			break;
		}
		case EClassFilterState::None: {
			FScopedTransaction Transaction(LOCTEXT("ClassFilter_UnmarkClass", "Unmark Class"));
			if (Class)
			{
				Class->SetOwnFilterState(State);

				if (PropertyHandle)
					PropertyHandle->NotifyPreChange();
				for (const auto& Filter : Filters)
				{
					Filter.Filter->IgnoredClasses.Remove(ClassAsset);
					Filter.Filter->AllowedClasses.Remove(ClassAsset);
				}
				if (PropertyHandle)
					PropertyHandle->NotifyPostChange(EPropertyChangeType::Unspecified);
			}
			break;
		}
	}
	OnFilterChanged.ExecuteIfBound();
}

FReply SClassFilter::OnClickedClearAll()
{
	FScopedTransaction Transaction(LOCTEXT("GameplayTagWidget_RemoveAllTags", "Remove All Gameplay Tags"));

	for (int32 ContainerIdx = 0; ContainerIdx < Filters.Num(); ++ContainerIdx)
	{
		UObject* OwnerObj = Filters[ContainerIdx].Owner.Get();
		FSEClassFilter* Container = Filters[ContainerIdx].Filter;

		if (Container)
		{
			FSEClassFilter EmptyFilter;
			SetFilter(Container, &EmptyFilter, OwnerObj);
		}
	}
	return FReply::Handled();
}

FSlateColor SClassFilter::GetClassBackgroundColor(FSEClassFilterNodePtr Class) const
{
	FLinearColor Color;
	const EClassFilterState OwnState = Class->GetOwnFilterState();
	const EClassFilterState ParentState = Class->GetParentFilterState();
	if (OwnState == EClassFilterState::Allowed)
	{
		Color = FLinearColor::Green;
	}
	else if (OwnState == EClassFilterState::Denied)
	{
		Color = FLinearColor::Red;
	}
	else if (ParentState == EClassFilterState::Allowed)
	{
		Color = FLinearColor::Green;
	}
	else if (ParentState == EClassFilterState::Denied)
	{
		Color = FLinearColor::Red;
	}
	else
	{
		return {FLinearColor::Transparent};
	}
	Color.A = 0.3f;
	return {Color};
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
	for (const auto& Child : RootClasses)
	{
		SetTreeItemExpansion(Child, bExpand);
	}
}

void SClassFilter::SetTreeItemExpansion(FSEClassFilterNodePtr Node, bool bExpand)
{
	if (Node.IsValid() && TreeWidget.IsValid())
	{
		TreeWidget->SetItemExpansion(Node, bExpand);

		for (const auto& Child : Node->GetChildrenList())
		{
			SetTreeItemExpansion(Child, bExpand);
		}
	}
}

void SClassFilter::SetDefaultTreeItemExpansion(FSEClassFilterNodePtr Node)
{
	if (Node.IsValid() && TreeWidget.IsValid())
	{
		bool bExpanded = false;

		/*if (IsClassAllowedOrDenied(Node) == ECheckBoxState::Checked)
		{
			bExpanded = true;
		}*/
		TreeWidget->SetItemExpansion(Node, bExpanded);

		for (const auto& Child : Node->GetChildrenList())
		{
			SetDefaultTreeItemExpansion(Child);
		}
	}
}

void SClassFilter::SetFilter(FSEClassFilter* OriginalFilter, FSEClassFilter* EditedFilter, UObject* OwnerObj)
{
	if (PropertyHandle.IsValid() && bMultiSelect)
	{
		// Case for a tag container
		// PropertyHandle->SetValueFromFormattedString(EditedFilter->ToString());
	}
	else if (PropertyHandle.IsValid() && !bMultiSelect)
	{
		// Case for a single Tag
		// FString FormattedString = TEXT("(TagName=\"");
		// FormattedString += EditedFilter.First().GetTagName().ToString();
		// FormattedString += TEXT("\")");
		// PropertyHandle->SetValueFromFormattedString(FormattedString);
	}
	else
	{
		// Not sure if we should get here, means the property handle hasn't been setup which could be right or
		// wrong.
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

void SClassFilter::Refresh()
{
	bNeedsRefresh = true;
}

void SClassFilter::Tick(
	const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	// Will populate the class hierarchy if needed
	ClassFilter::Helpers::PopulateClassHierarchy();

	if (bNeedsRefresh)
	{
		bNeedsRefresh = false;
		Populate();
	}
}

int32 SClassFilter::CountTreeItems(FSEClassFilterNode* Node)
{
	if (!Node)
		return 0;

	int32 Count = 1;
	for (const auto& Child : Node->GetChildrenList())
	{
		Count += CountTreeItems(Child.Get());
	}
	return Count;
}

void SClassFilter::Populate()
{
	// Empty the tree out so it can be redone.
	RootClasses.Empty();

	TArray<FSoftClassPath> InternalClassNames;
	TArray<UClass*> InternalClasses;
	TArray<FDirectoryPath> InternalPaths;

	// We aren't showing the internal classes, then we need to know what classes to consider Internal Only, so
	// let's gather them up from the settings object.
	GetInternalOnlyPaths(InternalPaths);
	GetInternalOnlyClasses(InternalClassNames);

	// Take the package names for the internal only classes and convert them into their UClass
	for (int i = 0; i < InternalClassNames.Num(); i++)
	{
		FString PackageClassName = InternalClassNames[i].ToString();
		const FSEClassFilterNodePtr ClassNode = ClassFilter::Helpers::ClassHierarchy->FindNodeByClassName(
			ClassFilter::Helpers::ClassHierarchy->GetObjectRootNode(), PackageClassName);

		if (ClassNode.IsValid())
		{
			InternalClasses.Add(ClassNode->Class.Get());
		}
	}


	// The root node for the tree, will be "Object" which we will skip.
	FSEClassFilterNodePtr RootNode;

	// Get the class tree, passing in certain filter options.
	ClassFilter::Helpers::GetClassTree(
		*Filters[0].Filter, RootNode, false, true, false, InternalClasses, InternalPaths);

	// Add all the children of the "Object" root.
	for (const auto& Child : RootNode->GetChildrenList())
	{
		RootClasses.Add(Child);
	}

	// Now that new items are in the tree, we need to request a refresh.
	TreeWidget->RequestTreeRefresh();
}

void SClassFilter::GetInternalOnlyClasses(TArray<FSoftClassPath>& Classes)
{
	Classes = GetDefault<UClassViewerProjectSettings>()->InternalOnlyClasses;
}

void SClassFilter::GetInternalOnlyPaths(TArray<FDirectoryPath>& Paths)
{
	Paths = GetDefault<UClassViewerProjectSettings>()->InternalOnlyPaths;
}

EVisibility SClassFilter::DetermineClearSelectionVisibility() const
{
	return CanSelectClasses() ? EVisibility::Visible : EVisibility::Collapsed;
}

void SClassFilter::OnExpansionChanged(FSEClassFilterNodePtr Class, bool bIsExpanded) {}

bool SClassFilter::CanSelectClasses() const
{
	return !bReadOnly;
}

TSharedPtr<SWidget> SClassFilter::GetWidgetToFocusOnOpen()
{
	return SearchBox;
}

#undef LOCTEXT_NAMESPACE
