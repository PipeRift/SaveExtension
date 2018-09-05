// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "PropertyEditorModule.h"
#include "ISaveExtensionEditor.h"


class FSaveExtensionEditor : public ISaveExtensionEditor
{
public:

	virtual void StartupModule() override;

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
};

IMPLEMENT_MODULE(FSaveExtensionEditor, SaveExtensionEditor);
