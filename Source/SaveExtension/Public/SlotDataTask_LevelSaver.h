// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "ISaveExtension.h"

#include "SavePreset.h"

#include "SlotDataTask_Saver.h"
#include "SlotDataTask_LevelSaver.generated.h"


/**
* Manages the serializing process of a single level
*/
UCLASS()
class USlotDataTask_LevelSaver : public USlotDataTask_Saver
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
	virtual void OnFinish(bool bSuccess) override  {
		SELog(*Settings, "Finished Serializing level", FColor::Green);
	}
};
