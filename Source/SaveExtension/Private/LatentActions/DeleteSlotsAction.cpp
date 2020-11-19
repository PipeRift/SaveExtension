// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "LatentActions/DeleteSlotsAction.h"

#include "SaveManager.h"
#include "SlotInfo.h"


FDeleteSlotsAction::FDeleteSlotsAction(USaveManager* Manager, EDeleteSlotsResult& OutResult, const FLatentActionInfo& LatentInfo)
	: Result(OutResult)
	, bFinished(false)
	, ExecutionFunction(LatentInfo.ExecutionFunction)
	, OutputLink(LatentInfo.Linkage)
	, CallbackTarget(LatentInfo.CallbackTarget)
{
	Manager->DeleteAllSlots(FOnSlotsDeleted::CreateLambda([this]() {
		bFinished = true;
	}));
}

void FDeleteSlotsAction::UpdateOperation(FLatentResponse& Response)
{
	Response.FinishAndTriggerIf(bFinished, ExecutionFunction, OutputLink, CallbackTarget);
}
