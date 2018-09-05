// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include <Interface.h>
#include "SaveExtensionInterface.generated.h"

UINTERFACE(Category = SaveExtension, BlueprintType)
class SAVEEXTENSION_API USaveExtensionInterface : public UInterface {
	GENERATED_UINTERFACE_BODY()
};

class SAVEEXTENSION_API ISaveExtensionInterface {

	GENERATED_IINTERFACE_BODY()

public:

	// Event called when Save process starts
	UFUNCTION(Category = Save, BlueprintImplementableEvent)
	void OnSaveBegan();

	// Event called when Save process ends
	UFUNCTION(Category = Save, BlueprintImplementableEvent)
	void OnSaveFinished(bool bError);

	// Event called when Load process starts
	UFUNCTION(Category = Save, BlueprintImplementableEvent)
	void OnLoadBegan();

	// Event called when Load process ends
	UFUNCTION(Category = Save, BlueprintImplementableEvent)
	void OnLoadFinished(bool bError);
};
