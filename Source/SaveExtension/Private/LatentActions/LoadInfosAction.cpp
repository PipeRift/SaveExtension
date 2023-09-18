// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "LatentActions/LoadInfosAction.h"

#include "SaveManager.h"
#include "SaveSlot.h"


FLoadInfosAction::FLoadInfosAction(USaveManager* Manager, const bool bSortByRecent,
	TArray<USaveSlot*>& InSlots, ELoadInfoResult& OutResult, const FLatentActionInfo& LatentInfo)
	: Result(OutResult)
	, Slots(InSlots)
	, bFinished(false)
	, ExecutionFunction(LatentInfo.ExecutionFunction)
	, OutputLink(LatentInfo.Linkage)
	, CallbackTarget(LatentInfo.CallbackTarget)
{
	Manager->FindAllSlots(
		bSortByRecent, FOnSlotsLoaded::CreateLambda([this](const TArray<USaveSlot*>& Results) {
			Slots = Results;
			bFinished = true;
		}));
}

void FLoadInfosAction::UpdateOperation(FLatentResponse& Response)
{
	Response.FinishAndTriggerIf(bFinished, ExecutionFunction, OutputLink, CallbackTarget);
}
