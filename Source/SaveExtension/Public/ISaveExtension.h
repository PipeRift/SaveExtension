// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "Engine/Engine.h"
#include "SavePreset.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSaveExtension, All, All);


class ISaveExtension : public IModuleInterface {
public:
	static inline ISaveExtension& Get() {
		return FModuleManager::LoadModuleChecked<ISaveExtension>("SaveExtension");
	}
	static inline bool IsAvailable() {
		return FModuleManager::Get().IsModuleLoaded("SaveExtension");
	}

	static void Log(const USavePreset* Preset, const FString Message, bool bError)
	{
		Log(Preset, Message, FColor::White, bError, 2.f);
	}

	static void Log(const USavePreset* Preset, const FString Message, FColor Color = FColor::White, bool bError = false, const float Duration = 2.f)
	{
		if (Preset->bDebug)
		{
			if (bError) {
				Color = FColor::Red;
			}

			const FString ComposedMessage { FString::Printf(TEXT("SE: %s"), *Message) };

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
	}
};

//Only log in Editor
#if WITH_EDITORONLY_DATA
#define SE_LOG(...) ISaveExtension::Log(##__VA_ARGS__)
#else
#define SE_LOG(...)
#endif
