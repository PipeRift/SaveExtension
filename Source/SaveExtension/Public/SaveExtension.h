// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "Engine/Engine.h"
#include "Modules/ModuleManager.h"
#include "SaveSlot.h"


DECLARE_LOG_CATEGORY_EXTERN(LogSaveExtension, All, All);


class FSaveExtension : public IModuleInterface
{
public:
	void StartupModule() override {}
	void ShutdownModule() override {}
	bool SupportsDynamicReloading() override
	{
		return true;
	}

	static inline FSaveExtension& Get()
	{
		return FModuleManager::LoadModuleChecked<FSaveExtension>("SaveExtension");
	}
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("SaveExtension");
	}

	static void Log(const USaveSlot* Slot, const FString& Message, bool bError)
	{
		Log(Slot, Message, FColor::White, bError, 2.f);
	}

	static void Log(const USaveSlot* Slot, const FString& Message, FColor Color = FColor::White,
		bool bError = false, const float Duration = 2.f);
};

// Only log in Editor
#if WITH_EDITORONLY_DATA
template <typename... Args>
void SELog(Args&&... args)
{
	FSaveExtension::Log(Forward<Args>(args)...);
}
#else
template <typename... Args>
void SELog(Args&&... args)
{}	  // Optimized away by compiler
#endif
