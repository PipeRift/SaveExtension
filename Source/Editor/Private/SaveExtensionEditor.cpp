// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "SaveExtensionEditor.h"

#include "Kismet2/KismetEditorUtilities.h"

#include "Asset/AssetTypeAction_SlotInfo.h"
#include "Asset/AssetTypeAction_SlotData.h"
#include "Asset/AssetTypeAction_SavePreset.h"

#include "Customizations/SavePresetDetails.h"
#include "Customizations/ClassFilterCustomization.h"
#include "Customizations/ClassFilterGraphPanelPinFactory.h"

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

	BlueprintEditorTabBinding = MakeShared<FSaveActorEditorTabBinding>();
}

void FSaveExtensionEditor::ShutdownModule()
{
	BlueprintEditorTabBinding = nullptr;

	// Unregister all pin customizations
	for (auto& FactoryPtr : CreatedPinFactories)
	{
		FEdGraphUtilities::UnregisterVisualPinFactory(FactoryPtr);
	}
	CreatedPinFactories.Empty();
}

void FSaveExtensionEditor::RegisterPropertyTypeCustomizations()
{
	RegisterCustomClassLayout("SavePreset", FOnGetDetailCustomizationInstance::CreateStatic(&FSavePresetDetails::MakeInstance));

	RegisterCustomPropertyTypeLayout("ClassFilter", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FClassFilterCustomization::MakeInstance));
	//RegisterCustomPropertyTypeLayout("SavePreset", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSavePresetCustomization::MakeInstance));

	RegisterCustomPinFactory<FClassFilterGraphPanelPinFactory>();
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

template<class T>
void FSaveExtensionEditor::RegisterCustomPinFactory()
{
	TSharedPtr<T> PinFactory = MakeShareable(new T());
	FEdGraphUtilities::RegisterVisualPinFactory(PinFactory);
	CreatedPinFactories.Add(PinFactory);
}

#undef LOCTEXT_NAMESPACE
