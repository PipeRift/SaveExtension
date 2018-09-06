// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "SaveExtensionEditor.h"

#include "Kismet2/KismetEditorUtilities.h"

#include "Asset/AssetTypeAction_SlotInfo.h"
#include "Asset/AssetTypeAction_SlotData.h"
#include "Asset/AssetTypeAction_SavePreset.h"
//#include "Customizations/SavePresetCustomization.h"
#include "Customizations/SavePresetDetails.h"

#define LOCTEXT_NAMESPACE "SaveExtensionEditor"


void FSaveExtensionEditor::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName("SaveExtension"), LOCTEXT("Category", "Save Extension"));

	{
		TSharedRef<IAssetTypeActions> SaveInfoAction = MakeShareable(new FAssetTypeAction_SlotInfo);
		AssetTools.RegisterAssetTypeActions(SaveInfoAction);

		TSharedRef<IAssetTypeActions> SaveDataAction = MakeShareable(new FAssetTypeAction_SlotData);
		AssetTools.RegisterAssetTypeActions(SaveDataAction);

		TSharedRef<IAssetTypeActions> SavePresetAction = MakeShareable(new FAssetTypeAction_SavePreset);
		AssetTools.RegisterAssetTypeActions(SavePresetAction);
	}

	RegisterPropertyTypeCustomizations();
}


void FSaveExtensionEditor::RegisterPropertyTypeCustomizations()
{
	RegisterCustomClassLayout("SavePreset", FOnGetDetailCustomizationInstance::CreateStatic(&FSavePresetDetails::MakeInstance));

	//RegisterCustomPropertyTypeLayout("SavePreset", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSavePresetCustomization::MakeInstance));
}

void FSaveExtensionEditor::RegisterCustomClassLayout(FName ClassName, FOnGetDetailCustomizationInstance DetailLayoutDelegate)
{
	check(ClassName != NAME_None);

	static FName PropertyEditor("PropertyEditor");
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);
	PropertyModule.RegisterCustomClassLayout(ClassName, DetailLayoutDelegate);
}

void FSaveExtensionEditor::RegisterCustomPropertyTypeLayout(FName PropertyTypeName, FOnGetPropertyTypeCustomizationInstance PropertyTypeLayoutDelegate)
{
	check(PropertyTypeName != NAME_None);

	static FName PropertyEditor("PropertyEditor");
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);
	PropertyModule.RegisterCustomPropertyTypeLayout(PropertyTypeName, PropertyTypeLayoutDelegate);
}

#undef LOCTEXT_NAMESPACE
