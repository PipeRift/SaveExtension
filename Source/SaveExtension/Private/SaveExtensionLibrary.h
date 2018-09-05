// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "Runtime/Engine/Classes/Kismet/BlueprintFunctionLibrary.h"

#include "SlotData.h"
//#include "SaveTimerHandle.h"

#include "SaveExtensionLibrary.generated.h"


UCLASS()
class SAVEEXTENSION_API USaveExtensionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	//UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Saveable Timer by Event"), Category = "Utilities|Time")
	//static FSaveTimerHandle SetSaveTimerDelegate(FTimerDynamicDelegate Delegate, float Time, bool bLooping);
};
