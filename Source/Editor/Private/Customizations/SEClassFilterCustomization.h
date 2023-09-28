// Copyright 2015-2024 Piperift. All Rights Reserved.
#pragma once

#include "ClassFilter/SClassFilter.h"

#include <EditorUndoClient.h>
#include <IPropertyTypeCustomization.h>


class IPropertyHandle;

struct FSEClassFilterItem
{
	FString ClassName;
	bool bAllowed;

	FSEClassFilterItem(FString ClassName, bool bAllowed) : ClassName{ClassName}, bAllowed{bAllowed} {}
};


class FSEClassFilterCustomization : public IPropertyTypeCustomization, public FEditorUndoClient
{
protected:
	/** Filters this customization edits */
	TArray<SClassFilter::FEditableClassFilterDatum> EditableFilters;
	TArray<TSharedPtr<FSEClassFilterItem>> PreviewClasses;

	TSharedPtr<IPropertyHandle> StructHandle;

	TSharedPtr<class SComboButton> EditButton;

	TSharedPtr<SListView<TSharedPtr<FSEClassFilterItem>>> PreviewList;

	TWeakPtr<SClassFilter> LastFilterPopup;

public:
	/**
	 * Creates a new instance.
	 *
	 * @return A new struct customization for Factions.
	 */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShared<FSEClassFilterCustomization>();
	}

	~FSEClassFilterCustomization();

protected:
	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle,
		class FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle,
		class IDetailChildrenBuilder& StructBuilder,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	//~ End FEditorUndoClient Interface


	/** Build List of Editable Containers */
	void BuildEditableFilterList();

	TSharedRef<SWidget> GetListContent();

	void OnPopupStateChanged(bool bIsOpened);

	void OnClearClicked();


	EVisibility GetClassPreviewVisibility() const;

	TSharedRef<SWidget> GetClassPreview();

	TSharedRef<ITableRow> OnGeneratePreviewRow(
		TSharedPtr<FSEClassFilterItem> Class, const TSharedRef<STableViewBase>& OwnerTable);


	void RefreshClassList();
};
