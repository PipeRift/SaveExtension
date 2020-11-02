// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include "SavePreset.h"

#include <CoreMinimal.h>
#include <Engine/DeveloperSettings.h>

#include "SaveSettings.generated.h"


UCLASS(ClassGroup = SaveExtension, defaultconfig, config = Game, meta=(DisplayName="Save Extension"))
class SAVEEXTENSION_API USaveSettings : public UDeveloperSettings
{
    GENERATED_BODY()

protected:

    UPROPERTY(EditAnywhere, Category = "Save Extension", Config, meta = (DisplayName = "Preset"))
	TSubclassOf<USavePreset> Preset;

public:

    // If true SaveManager will tick with the world. If game is paused, saving process may be interrupted.
    UPROPERTY(EditAnywhere, Category = "Save Extension", Config)
    bool bTickWithGameWorld = false;


    USavePreset* CreatePreset(UObject* Outer) const;
};


inline USavePreset* USaveSettings::CreatePreset(UObject* Outer) const
{
    if(UClass* PresetClass = Preset.Get())
	{
        return NewObject<USavePreset>(Outer, PresetClass);
	}
    return NewObject<USavePreset>(Outer);
}
