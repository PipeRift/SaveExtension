// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include <Modules/ModuleManager.h>
#include <Engine/Engine.h>

#include "SavePreset.h"


DECLARE_LOG_CATEGORY_EXTERN(LogSaveExtension, All, All);

class ISaveExtension : public IModuleInterface
{
public:
	static inline ISaveExtension& Get()
	{
		return FModuleManager::LoadModuleChecked<ISaveExtension>("SaveExtension");
	}
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("SaveExtension");
	}

	static void Log(const USavePreset* Preset, const FString& Message, bool bError)
	{
		Log(Preset, Message, FColor::White, bError, 2.f);
	}

	static void Log(const USavePreset* Preset, const FString& Message, FColor Color = FColor::White, bool bError = false, const float Duration = 2.f)
	{
		if (Preset->bDebug)
		{
			if (bError)
			{
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
template <typename ...Args>
void SELog(Args&& ...args) { ISaveExtension::Log(Forward<Args>(args)...); }
#else
template <typename ...Args>
void SELog(Args&& ...args) {} // Optimized away by compiler
#endif
