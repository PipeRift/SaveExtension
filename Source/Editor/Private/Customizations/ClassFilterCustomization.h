// Copyright 2015-2019 Piperift. All Rights Reserved.
#pragma once

#include <IPropertyTypeCustomization.h>
#include <EditorUndoClient.h>
#include "ClassFilter/SClassFilter.h"


class IPropertyHandle;

struct FClassFilterItem {
	FString ClassName;
	bool bAllowed;

	FClassFilterItem(FString ClassName, bool bAllowed)
		: ClassName{ClassName}, bAllowed{bAllowed}
	{}
};


class FClassFilterCustomization : public IPropertyTypeCustomization, public FEditorUndoClient
{
protected:

	/** Filters this customization edits */
	TArray<SClassFilter::FEditableClassFilterDatum> EditableFilters;
	TArray<TSharedPtr<FClassFilterItem>> PreviewClasses;

	TSharedPtr<IPropertyHandle> StructHandle;

	TSharedPtr<class SComboButton> EditButton;

	TSharedPtr<SListView<TSharedPtr<FClassFilterItem>>> PreviewList;

	TWeakPtr<SClassFilter> LastFilterPopup;

public:

	/**
	* Creates a new instance.
	*
	* @return A new struct customization for Factions.
	*/
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShared<FClassFilterCustomization>();
	}

	~FClassFilterCustomization();

protected:

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	//~ End FEditorUndoClient Interface



	/** Build List of Editable Containers */
	void BuildEditableFilterList();

	TSharedRef<SWidget> GetListContent();

	void OnPopupStateChanged(bool bIsOpened);

	FReply OnClearClicked();


	EVisibility GetClassPreviewVisibility() const;

	TSharedRef<SWidget> GetClassPreview();

	TSharedRef<ITableRow> OnGeneratePreviewRow(TSharedPtr<FClassFilterItem> Class, const TSharedRef<STableViewBase>& OwnerTable);


	void RefreshClassList();
};

