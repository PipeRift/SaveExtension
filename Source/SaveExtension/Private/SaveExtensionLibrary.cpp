// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "SaveExtensionLibrary.h"

#include "CoreGlobals.h"
#include "HighResScreenshot.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SaveManager.h"



/*FSaveTimerHandle USaveExtensionLibrary::SetSaveTimerDelegate(FTimerDynamicDelegate Delegate, float Time,
bool bLooping)
{
	FTimerHandle Handle { UKismetSystemLibrary::K2_SetTimerDelegate(Delegate, Time, bLooping) };
	return FSaveTimerHandle(Handle, Delegate, Time, bLooping);
}*/
