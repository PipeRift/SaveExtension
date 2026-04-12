// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "AssetTypeCategories.h"
#include "Modules/ModuleManager.h"



class ISaveExtensionEditor : public IModuleInterface
{
public:
	EAssetTypeCategories::Type AssetCategory;

	static inline ISaveExtensionEditor& Get()
	{
		return FModuleManager::LoadModuleChecked<ISaveExtensionEditor>("SaveExtensionEditor");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("SaveExtensionEditor");
	}
};
