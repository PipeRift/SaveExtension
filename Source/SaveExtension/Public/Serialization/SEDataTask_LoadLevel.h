// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "ISaveExtension.h"
#include "SEDataTask_Load.h"


/**
 * Manages the serializing process of a single level
 */
struct FSEDataTask_LoadLevel : public FSEDataTask_Load
{
	TObjectPtr<ULevelStreaming> StreamingLevel;


public:
	FSEDataTask_LoadLevel(USaveManager* Manager, USaveSlot* Slot)
		: FSEDataTask_Load(Manager, Slot)
	{}

	auto& Setup(ULevelStreaming* InStreamingLevel)
	{
		StreamingLevel = InStreamingLevel;
		return *this;
	}

private:
	virtual void OnStart() override;

	virtual void DeserializeASyncLoop(float StartMS = 0.0f) override;
};
