// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "SEDataTask_Save.h"


/**
 * Manages the serializing process of a single level
 */
struct FSEDataTask_SaveLevel : public FSEDataTask_Save
{
	TObjectPtr<ULevelStreaming> StreamingLevel;


public:
	FSEDataTask_SaveLevel(USaveManager* Manager, USaveSlot* Slot)
		: FSEDataTask_Save(Manager, Slot)
	{}

	auto& Setup(ULevelStreaming* InStreamingLevel)
	{
		StreamingLevel = InStreamingLevel;
		return *this;
	}

private:
	virtual void OnStart() override;
	virtual void OnFinish(bool bSuccess) override;
};
