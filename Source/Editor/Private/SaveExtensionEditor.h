// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include <PropertyEditorModule.h>
#include <Framework/Docking/LayoutExtender.h>
#include <WorkflowOrientedApp/WorkflowTabManager.h>
#include <BlueprintEditorModes.h>
#include <BlueprintEditorModule.h>
#include <BlueprintEditorTabs.h>

#include "SaveActorEditorTabSummoner.h"
#include "ISaveExtensionEditor.h"


/** Shared class type that ensures safe binding to RegisterBlueprintEditorTab through an SP binding without interfering with module ownership semantics */
class FSaveActorEditorTabBinding
	: public TSharedFromThis<FSaveActorEditorTabBinding>
{
public:

	FSaveActorEditorTabBinding()
	{
		FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
		BlueprintEditorTabSpawnerHandle = BlueprintEditorModule.OnRegisterTabsForEditor().AddRaw(this, &FSaveActorEditorTabBinding::RegisterBlueprintEditorTab);
		BlueprintEditorLayoutExtensionHandle = BlueprintEditorModule.OnRegisterLayoutExtensions().AddRaw(this, &FSaveActorEditorTabBinding::RegisterBlueprintEditorLayout);
	}

	void RegisterBlueprintEditorLayout(FLayoutExtender& Extender)
	{
		Extender.ExtendLayout(FBlueprintEditorTabs::MyBlueprintID, ELayoutExtensionPosition::After, FTabManager::FTab(FSaveActorEditorSummoner::TabName, ETabState::OpenedTab));
	}

	void RegisterBlueprintEditorTab(FWorkflowAllowedTabSet& TabFactories, FName InModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor)
	{
		TabFactories.RegisterFactory(MakeShared<FSaveActorEditorSummoner>(BlueprintEditor));
	}

	~FSaveActorEditorTabBinding()
	{
		FBlueprintEditorModule* BlueprintEditorModule = FModuleManager::GetModulePtr<FBlueprintEditorModule>("Kismet");
		if (BlueprintEditorModule)
		{
			BlueprintEditorModule->OnRegisterTabsForEditor().Remove(BlueprintEditorTabSpawnerHandle);
			BlueprintEditorModule->OnRegisterLayoutExtensions().Remove(BlueprintEditorLayoutExtensionHandle);
		}
	}

private:

	/** Delegate binding handle for FBlueprintEditorModule::OnRegisterTabsForEditor */
	FDelegateHandle BlueprintEditorTabSpawnerHandle, BlueprintEditorLayoutExtensionHandle;
};

class FSaveExtensionEditor : public ISaveExtensionEditor
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

	void RegisterPropertyTypeCustomizations();

	/**
	* Registers a custom class
	*
	* @param ClassName				The class name to register for property customization
	* @param DetailLayoutDelegate	The delegate to call to get the custom detail layout instance
	*/
	void RegisterCustomClassLayout(FName ClassName, FOnGetDetailCustomizationInstance DetailLayoutDelegate);

	/**
	* Registers a custom struct
	*
	* @param StructName				The name of the struct to register for property customization
	* @param StructLayoutDelegate	The delegate to call to get the custom detail layout instance
	*/
	void RegisterCustomPropertyTypeLayout(FName PropertyTypeName, FOnGetPropertyTypeCustomizationInstance PropertyTypeLayoutDelegate);


	TSharedPtr<FSaveActorEditorTabBinding> BlueprintEditorTabBinding;
};

IMPLEMENT_MODULE(FSaveExtensionEditor, SaveExtensionEditor);
