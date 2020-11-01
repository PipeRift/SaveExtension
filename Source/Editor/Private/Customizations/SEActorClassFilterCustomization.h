// Copyright 2015-2020 Piperift. All Rights Reserved.
#pragma once

#include "SEClassFilterCustomization.h"


class IPropertyHandle;

class FSEActorClassFilterCustomization : public FSEClassFilterCustomization
{
public:

	/**
	* Creates a new instance.
	*
	* @return A new struct customization for Factions.
	*/
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShared<FSEActorClassFilterCustomization>();
	}

protected:

	virtual TSharedPtr<IPropertyHandle> GetFilterHandle(TSharedRef<IPropertyHandle> StructPropertyHandle) override;
};

