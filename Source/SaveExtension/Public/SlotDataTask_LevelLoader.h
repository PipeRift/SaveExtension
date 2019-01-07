// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "ISaveExtension.h"

#include "SavePreset.h"

#include "SlotDataTask_Loader.h"
#include "SlotDataTask_LevelLoader.generated.h"


/**
* Manages the serializing process of a single level
*/
UCLASS()
class USlotDataTask_LevelLoader : public USlotDataTask_Loader
{
	GENERATED_BODY()


	UPROPERTY()
	ULevelStreaming* StreamingLevel;

public:

	auto Setup(ULevelStreaming* InStreamingLevel)
	{
		StreamingLevel = InStreamingLevel;
		return this;
	}

private:

	virtual void OnStart() override;

	virtual void DeserializeASyncLoop(float StartMS = 0.0f) override;
};
