// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "SaveExtensionEditor.h"

#include <AssetTypeActions_Base.h>
#include <SaveSlot.h>


class FAssetTypeAction_SaveSlot : public FAssetTypeActions_Base
{
public:
	virtual uint32 GetCategories() override
	{
		return FSaveExtensionEditor::Get().AssetCategory;
	}

	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;

	virtual UClass* GetSupportedClass() const override
	{
		return USaveSlot::StaticClass();
	}
};

#undef LOCTEXT_NAMESPACE
