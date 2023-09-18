// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "ISaveExtension.h"
#include "SlotDataTask_Saver.h"

#include "SlotDataTask_LevelSaver.generated.h"


/**
 * Manages the serializing process of a single level
 */
UCLASS()
class USaveSlotDataTask_LevelSaver : public USaveSlotDataTask_Saver
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
	virtual void OnFinish(bool bSuccess) override
	{
		SELog(Slot, "Finished Serializing level", FColor::Green);
	}
};
