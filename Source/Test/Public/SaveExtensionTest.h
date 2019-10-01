// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"


class FSaveExtensionTest : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static inline FSaveExtensionTest& Get() {
		return FModuleManager::LoadModuleChecked<FSaveExtensionTest>("SaveExtensionTest");
	}

	static inline bool IsAvailable() {
		return FModuleManager::Get().IsModuleLoaded("SaveExtensionTest");
	}
};

IMPLEMENT_MODULE(FSaveExtensionTest, SaveExtensionTest);
