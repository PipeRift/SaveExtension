// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "ISaveExtension.h"
#include "SaveManager.h"
#include "SavePreset.h"
#include "SlotData.h"

#if WITH_EDITORONLY_DATA
 #include "ISettingsModule.h"
 #include "ISettingsSection.h"
 #include "ISettingsContainer.h"
#endif

#define LOCTEXT_NAMESPACE "SaveExtension"


class FSaveExtension : public ISaveExtension
{
private:

#if WITH_EDITORONLY_DATA
	bool HandleSettingsSaved()
	{
		USavePreset* Settings = GetMutableDefault<USavePreset>();
		bool ResaveSettings = false;

		// You can put any validation code in here and resave the settings in case an invalid
		// value has been entered

		if (ResaveSettings)
		{
			Settings->SaveConfig();
		}

		return true;
	}
#endif

	void RegisterSettings()
	{
#if WITH_EDITORONLY_DATA
		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings")) {

			ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings("Project", "Game", "SaveExtension",
				LOCTEXT("SaveExtensionName", "Save Extension"),
				LOCTEXT("SaveExtensionDescription", "Save Extension settings"),
				GetMutableDefault<USaveManager>());

			if (SettingsSection.IsValid()) {
				SettingsSection->OnModified().BindRaw(this, &FSaveExtension::HandleSettingsSaved);
			}

			ISettingsSectionPtr PresetSettingsSection = SettingsModule->RegisterSettings("Project", "Game", "SaveExtensionPreset",
				LOCTEXT("SaveExtensionPresetName", "Save Extension: Default Preset"),
				LOCTEXT("SaveExtensionPresetDescription", "Default Save Extension preset values"),
				GetMutableDefault<USavePreset>());

			if (PresetSettingsSection.IsValid()) {
				PresetSettingsSection->OnModified().BindRaw(this, &FSaveExtension::HandleSettingsSaved);
			}
		}
#endif
	}

	void UnregisterSettings() {
#if WITH_EDITORONLY_DATA
		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings")) {
			SettingsModule->UnregisterSettings("Project", "Game", "SaveExtension");
			SettingsModule->UnregisterSettings("Project", "Game", "SaveExtensionPreset");
		}
#endif
	}

public:

	virtual void StartupModule() override {
		RegisterSettings();
	}

	virtual void ShutdownModule() override {
		if (UObjectInitialized()) {
			UnregisterSettings();
		}
	}

	virtual bool SupportsDynamicReloading() override { return true; }
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_GAME_MODULE(FSaveExtension, SaveExtension);
