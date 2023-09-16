// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "LevelFilter.h"

#include <UObject/Interface.h>

#include "SaveExtensionInterface.generated.h"



UINTERFACE(Category = SaveExtension, BlueprintType)
class SAVEEXTENSION_API USaveExtensionInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class SAVEEXTENSION_API ISaveExtensionInterface
{
	GENERATED_BODY()

public:
	/** BP API **/

	// Event called when Save process starts
	UFUNCTION(Category = Save, BlueprintImplementableEvent, meta = (DisplayName = "On Save Began"))
	void ReceiveOnSaveBegan(const FSELevelFilter& Filter);

	// Event called when Save process ends
	UFUNCTION(Category = Save, BlueprintImplementableEvent, meta = (DisplayName = "On Save Finished"))
	void ReceiveOnSaveFinished(const FSELevelFilter& Filter, bool bError);

	// Event called when Load process starts
	UFUNCTION(Category = Save, BlueprintImplementableEvent, meta = (DisplayName = "On Load Began"))
	void ReceiveOnLoadBegan(const FSELevelFilter& Filter);

	// Event called when Load process ends
	UFUNCTION(Category = Save, BlueprintImplementableEvent, meta = (DisplayName = "On Load Finished"))
	void ReceiveOnLoadFinished(const FSELevelFilter& Filter, bool bError);


	/** C++ API **/

	// Event called when Save process starts
	virtual void OnSaveBegan(const FSELevelFilter& Filter) {}

	// Event called when Save process ends
	virtual void OnSaveFinished(const FSELevelFilter& Filter, bool bError) {}

	// Event called when Load process starts
	virtual void OnLoadBegan(const FSELevelFilter& Filter) {}

	// Event called when Load process ends
	virtual void OnLoadFinished(const FSELevelFilter& Filter, bool bError) {}
};
