// Copyright 2015-2019 Piperift. All Rights Reserved.
#pragma once

#include <IPropertyTypeCustomization.h>

class IPropertyHandle;

class FActorFilterCustomization : public IPropertyTypeCustomization
{
public:
	/**
	* Creates a new instance.
	*
	* @return A new struct customization for Factions.
	*/
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShared<FActorFilterCustomization>();
	}


protected:

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;


	TSharedPtr<IPropertyHandle> StructHandle;

	TSharedPtr<class SComboButton> EditButton;

	TSharedPtr<SWidget> GetListContent();
	void OnListMenuOpenStateChanged(bool bIsOpened);
};

