// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "Engine/Engine.h"
#include "SavePreset.h"
#include "CustomSaveGameSystem.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSaveExtension, All, All);


class ISaveExtension : public IModuleInterface {
public:
	static inline ISaveExtension& Get() {
		return FModuleManager::LoadModuleChecked<ISaveExtension>("SaveExtension");
	}
	static inline bool IsAvailable() {
		return FModuleManager::Get().IsModuleLoaded("SaveExtension");
	}

	FCustomSaveGameSystem* GetSaveSystem()
	{
		static FCustomSaveGameSystem System;
		return &System;
	}

	static void Log(const USavePreset* Preset, const FString Message, int8 Indent)
	{
		Log(Preset, Message, FColor::White, false, Indent, 2.f);
	}

	static void Log(const USavePreset* Preset, const FString Message, FColor Color = FColor::White, bool bError = false, int8 Indent = 0, const float Duration = 2.f)
	{
		if (!Preset->bDebug)
			return;

		if (bError) {
			Color = FColor::Red;
		}

		FString ComposedMessage = {};

		if (Indent <= 0) {
			ComposedMessage += "Save Extension: ";
		}
		else
		{
			for (int8 i{ 1 }; i <= Indent; ++i)
			{
				ComposedMessage += "  ";
			}
		}
		ComposedMessage += Message;

		if (bError)
		{
			UE_LOG(LogSaveExtension, Error, TEXT("%s"), *ComposedMessage);
		}
		else
		{
			UE_LOG(LogSaveExtension, Log, TEXT("%s"), *ComposedMessage);
		}

		if (Preset->bDebugInScreen && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, Duration, Color, ComposedMessage);
		}
	}
};

//Only log in Editor
#if WITH_EDITORONLY_DATA
#define SE_LOG(...) ISaveExtension::Log(##__VA_ARGS__)
#else
#define SE_LOG(...)
#endif
