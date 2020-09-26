// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include "SavePreset.h"

#include <CoreMinimal.h>

#include "SaveSettings.generated.h"


UCLASS(ClassGroup = SaveExtension, defaultconfig, config = Game, meta=(DisplayName="Save Extension"))
class SAVEEXTENSION_API USaveSettings : public UDeveloperSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Save Extension", Config, meta = (DisplayName = "Preset"))
	TSubclassOf<USavePreset> Preset;


public:
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
