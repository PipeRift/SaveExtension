// Copyright 2015-2019 Piperift. All Rights Reserved.
#pragma once

#include <IPropertyTypeCustomization.h>
#include "ClassFilter/SClassFilter.h"


class IPropertyHandle;

class FClassFilterCustomization : public IPropertyTypeCustomization
{
protected:

	/** Filters this customization edits */
	TArray<SClassFilter::FEditableClassFilterDatum> EditableFilters;

	TSharedPtr<IPropertyHandle> StructHandle;

	TSharedPtr<class SComboButton> EditButton;

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

protected:

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;


	/** Updates the list of classes */
	void RefreshClassesList();

	/** Build List of Editable Containers */
	void BuildEditableFilterList();

	TSharedRef<SWidget> GetListContent();

	void OnPopupStateChanged(bool bIsOpened);
};

