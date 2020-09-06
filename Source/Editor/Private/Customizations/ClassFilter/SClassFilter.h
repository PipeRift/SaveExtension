// Copyright 2015-2020 Piperift. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include <Engine/EngineTypes.h>
#include "SlateFwd.h"
#include "UObject/Object.h"
#include "Layout/Visibility.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STreeView.h"

#include "Misc/ClassFilter.h"
#include "ClassFilterNode.h"

class IPropertyHandle;


class SClassFilter : public SCompoundWidget
{
public:

	/** Called when the filter is changed */
	DECLARE_DELEGATE(FOnFilterChanged)

	SLATE_BEGIN_ARGS(SClassFilter)
		: _ReadOnly(false)
		, _MultiSelect(true)
		, _PropertyHandle(nullptr)
		, _MaxHeight(260.0f)
	{}
	SLATE_ARGUMENT(bool, ReadOnly) // Flag to set if the list is read only
		SLATE_ARGUMENT(bool, MultiSelect) // If we can select multiple entries
		SLATE_ARGUMENT(TSharedPtr<IPropertyHandle>, PropertyHandle)
		SLATE_EVENT(FOnFilterChanged, OnFilterChanged) // Called when a tag status changes
		SLATE_ARGUMENT(float, MaxHeight)	// caps the height of the gameplay tag tree
		SLATE_END_ARGS()

		/** Simple struct holding a tag container and its owner for generic re-use of the widget */
		struct FEditableClassFilterDatum
	{
		/** Constructor */
		FEditableClassFilterDatum(class UObject* InOwner, struct FClassFilter* InFilter)
			: Owner(InOwner)
			, Filter(InFilter)
		{}

		/** Owning UObject of the container being edited */
		TWeakObjectPtr<class UObject> Owner;

		/** Tag container to edit */
		struct FClassFilter* Filter;
	};

	/** Construct the actual widget */
	void Construct(const FArguments& InArgs, const TArray<FEditableClassFilterDatum>& EditableFilters);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	/** Ensures that this widget will always account for the MaxHeight if it's specified */
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

	/** Updates the tag list when the filter text changes */
	void OnSearchTextChanged(const FText& SearchText);

	/** Returns true if this TagNode has any children that match the current filter */
	bool FilterChildrenCheck(const FClassFilterNodePtr& Class);

	/** Gets the widget to focus once the menu opens. */
	TSharedPtr<SWidget> GetWidgetToFocusOnOpen();


private:

	FString SearchString;

	bool bReadOnly;

	/* Flag to set if we can select multiple items from the list*/
	bool bMultiSelect;

	/** The maximum height of the gameplay tag tree. If 0, the height is unbound. */
	float MaxHeight;

	/** True if the Class Filter needs to be repopulated at the next appropriate opportunity, occurs whenever classes are added, removed, renamed, etc. */
	bool bNeedsRefresh;

	/* Array of tags to be displayed in the TreeView */
	TArray<FClassFilterNodePtr> RootClasses;

	/* Array of tags filtered in the TreeView */
	TArray<FClassFilterNodePtr> FilteredClasses;

	/** Container widget holding the tag tree */
	TSharedPtr<SBorder> TreeContainerWidget;

	/** Tree widget showing the gameplay tag library */
	TSharedPtr<STreeView<FClassFilterNodePtr>> TreeWidget;

	/** Allows for the user to find a specific gameplay tag in the tree */
	TSharedPtr<SSearchBox> SearchBox;

	/** Containers to modify */
	TArray<FEditableClassFilterDatum> Filters;

	/** Called when the Class list changes*/
	FOnFilterChanged OnFilterChanged;

	TSharedPtr<IPropertyHandle> PropertyHandle;


	/**
	 * Generate a row widget for the specified item node and table
	 *
	 * @param InItem		Tag node to generate a row widget for
	 * @param OwnerTable	Table that owns the row
	 *
	 * @return Generated row widget for the item node
	 */
	TSharedRef<ITableRow> OnGenerateRow(FClassFilterNodePtr Class, const TSharedRef<STableViewBase>& OwnerTable);

	/**
	 * Get children nodes of the specified node
	 *
	 * @param InItem		Node to get children of
	 * @param OutChildren	[OUT] Array of children nodes, if any
	 */
	void OnGetChildren(FClassFilterNodePtr Class, TArray<FClassFilterNodePtr>& OutChildren);

	/**
	 * Called via delegate when the status of a check box in a row changes
	 *
	 * @param NewCheckState	New check box state
	 * @param NodeChanged	Node that was checked/unchecked
	 */
	FReply OnClassClicked(FClassFilterNodePtr Class);

	FText GetClassIconText(FClassFilterNodePtr Class) const;
	FSlateColor GetClassIconColor(FClassFilterNodePtr Class) const;

	void MarkClass(FClassFilterNodePtr Class, EClassFilterState State);

	FSlateColor GetClassBackgroundColor(FClassFilterNodePtr Class) const;

	/** Called when the user clicks the "Clear All" button; Clears all tags */
	FReply OnClickedClearAll();

	/** Called when the user clicks the "Expand All" button; Expands the entire tag tree */
	FReply OnClickedExpandAll();

	/** Called when the user clicks the "Collapse All" button; Collapses the entire tag tree */
	FReply OnClickedCollapseAll();

	/**
	 * Helper function to set the expansion state of the tree widget
	 *
	 * @param bExpand If true, expand the entire tree; Otherwise, collapse the entire tree
	 */
	void SetTreeItemExpansion(bool bExpand);

	/**
	 * Helper function to set the expansion state of a specific node
	 *
	 * @param Node		Node to set the expansion state of
	 * @param bExapnd	If true, expand the node; Otherwise, collapse the node
	 */
	void SetTreeItemExpansion(FClassFilterNodePtr Node, bool bExpand);

	/** Recursive function to go through all tags in the tree and set the expansion to default*/
	void SetDefaultTreeItemExpansion(FClassFilterNodePtr Node);

	/** Helper function to determine the visibility of the Clear Selection button */
	EVisibility DetermineClearSelectionVisibility() const;

	/** Expansion changed callback */
	void OnExpansionChanged(FClassFilterNodePtr Class, bool bIsExpanded);

	/** Returns true if the user can select tags from the widget */
	bool CanSelectClasses() const;

	void SetFilter(FClassFilter* OriginalContainer, FClassFilter* EditedContainer, UObject* OwnerObj);

	/** Sends a requests to the Class Viewer to refresh itself the next chance it gets */
	void Refresh();

	/** Count the number of tree items in the specified hierarchy*/
	int32 CountTreeItems(FClassFilterNode* Node);

	/** Populates the tree with items based on the current filter. */
	void Populate();

	/** Accessor for the class names that have been marked as internal only in settings */
	void GetInternalOnlyClasses(TArray<FSoftClassPath>& Classes);

	/** Accessor for the class paths that have been marked as internal only in settings */
	void GetInternalOnlyPaths(TArray<FDirectoryPath>& Paths);
};
