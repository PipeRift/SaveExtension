// Copyright 2015-2024 Piperift. All Rights Reserved.
#pragma once

#include "ClassFilter/SClassFilter.h"
#include "SEClassFilterCustomization.h"
#include "SGraphPin.h"

#include <CoreMinimal.h>
#include <Widgets/DeclarativeSyntaxSupport.h>
#include <Widgets/SWidget.h>
#include <Widgets/Views/STableRow.h>
#include <Widgets/Views/STableViewBase.h>



class SComboButton;

class SClassFilterGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SClassFilterGraphPin) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

	//~ Begin SGraphPin Interface
	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;
	//~ End SGraphPin Interface

private:
	/** Refreshes the list of tags displayed on the node. */
	void RefreshPreviewList();

	/** Parses the Data from the pin to fill in the names of the array. */
	void ParseDefaultValueData();

	/** Callback function to create content for the combo button. */
	TSharedRef<SWidget> GetListContent();

	/**
	 * Creates SelectedTags List.
	 * @return widget that contains the read only tag names for displaying on the node.
	 */
	TSharedRef<SWidget> SelectedTags();

	/**
	 * Callback for populating rows of the SelectedTags List View.
	 * @return widget that contains the name of a tag.
	 */
	TSharedRef<ITableRow> OnGeneratePreviewRow(
		TSharedPtr<FSEClassFilterItem> Class, const TSharedRef<STableViewBase>& OwnerTable);

private:
	// Combo Button for the drop down list.
	TSharedPtr<SComboButton> ComboButton;

	// Tag Container used for the Edit Widget.
	FSEClassFilter Filter;

	// Datum uses for the GameplayTagWidget.
	TArray<SClassFilter::FEditableClassFilterDatum> EditableFilters;

	// Array of names for the read only display of tag names on the node.
	TArray<TSharedPtr<FSEClassFilterItem>> PreviewClasses;

	// The List View used to display the read only tag names on the node.
	TSharedPtr<SListView<TSharedPtr<FSEClassFilterItem>>> PreviewList;
};
