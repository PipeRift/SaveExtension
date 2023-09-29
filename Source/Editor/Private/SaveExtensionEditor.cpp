// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "SaveExtensionEditor.h"

#include "Asset/AssetTypeAction_SaveSlot.h"
#include "Asset/AssetTypeAction_SaveSlotData.h"
#include "Customizations/SEClassFilterCustomization.h"
#include "Customizations/SEClassFilterGraphPanelPinFactory.h"
#include "Customizations/SaveSlotDetails.h"
#include "Kismet2/KismetEditorUtilities.h"


#define LOCTEXT_NAMESPACE "SaveExtensionEditor"


void FSaveExtensionEditor::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetCategory = AssetTools.RegisterAdvancedAssetCategory(
		FName("SaveExtension"), LOCTEXT("Category", "Save Extension"));

	AssetTools.RegisterAssetTypeActions(MakeShared<FAssetTypeAction_SaveSlot>());
	AssetTools.RegisterAssetTypeActions(MakeShared<FAssetTypeAction_SaveSlotData>());

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
	RegisterCustomClassLayout(
		"SaveSlot", FOnGetDetailCustomizationInstance::CreateStatic(&FSaveSlotDetails::MakeInstance));

	RegisterCustomPropertyTypeLayout("SEClassFilter",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSEClassFilterCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("SEActorClassFilter",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSEClassFilterCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("SEComponentClassFilter",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSEClassFilterCustomization::MakeInstance));

	RegisterCustomPinFactory<FSEClassFilterGraphPanelPinFactory>();
}

void FSaveExtensionEditor::RegisterCustomClassLayout(
	FName ClassName, FOnGetDetailCustomizationInstance DetailLayoutDelegate)
{
	check(ClassName != NAME_None);

	static FName PropertyEditor("PropertyEditor");
	FPropertyEditorModule& PropertyModule =
		FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);
	PropertyModule.RegisterCustomClassLayout(ClassName, DetailLayoutDelegate);
}

void FSaveExtensionEditor::RegisterCustomPropertyTypeLayout(
	FName PropertyTypeName, FOnGetPropertyTypeCustomizationInstance PropertyTypeLayoutDelegate)
{
	check(PropertyTypeName != NAME_None);

	static FName PropertyEditor("PropertyEditor");
	FPropertyEditorModule& PropertyModule =
		FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);
	PropertyModule.RegisterCustomPropertyTypeLayout(PropertyTypeName, PropertyTypeLayoutDelegate);
}

template <class T>
void FSaveExtensionEditor::RegisterCustomPinFactory()
{
	TSharedPtr<T> PinFactory = MakeShareable(new T());
	FEdGraphUtilities::RegisterVisualPinFactory(PinFactory);
	CreatedPinFactories.Add(PinFactory);
}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FSaveExtensionEditor, SaveExtensionEditor);