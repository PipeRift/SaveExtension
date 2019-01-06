// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "SaveExtensionEditor.h"

#include "AssetTypeActions_Base.h"

#include "SavePreset.h"

#define LOCTEXT_NAMESPACE "SaveExtensionEditor"


class FAssetTypeAction_SavePreset : public FAssetTypeActions_Base
{
public:

	virtual uint32 GetCategories() override {
		return FSaveExtensionEditor::Get().AssetCategory;
	}

	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;

	virtual UClass* GetSupportedClass() const override { return USavePreset::StaticClass(); }
};

#undef LOCTEXT_NAMESPACE
