// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "ISaveExtension.h"

#if WITH_EDITORONLY_DATA
#	include "ISettingsContainer.h"
#	include "ISettingsModule.h"
#	include "ISettingsSection.h"

#endif


class FSaveExtension : public ISaveExtension
{
public:
	virtual void StartupModule() override {}
	virtual void ShutdownModule() override {}

	virtual bool SupportsDynamicReloading() override
	{
		return true;
	}
};
