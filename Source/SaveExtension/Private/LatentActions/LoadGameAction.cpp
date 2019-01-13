// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "LatentActions/LoadGameAction.h"
#include "SaveManager.h"
#include "SlotInfo.h"

#include "SlotDataTask_Loader.h"


FLoadGameAction::FLoadGameAction(USaveManager* Manager, int32 SlotId, ELoadGameResult& OutResult, const FLatentActionInfo& LatentInfo)
	: Result(OutResult)
	, ExecutionFunction(LatentInfo.ExecutionFunction)
	, OutputLink(LatentInfo.Linkage)
	, CallbackTarget(LatentInfo.CallbackTarget)
{
	const bool bStarted = Manager->LoadSlot(SlotId, FOnGameLoaded::CreateRaw(this, &FLoadGameAction::OnLoadFinished));

	if (!bStarted)
		Result = ELoadGameResult::Failed;
}

void FLoadGameAction::UpdateOperation(FLatentResponse& Response)
{
	Response.FinishAndTriggerIf(Result != ELoadGameResult::Loading, ExecutionFunction, OutputLink, CallbackTarget);
}

void FLoadGameAction::OnLoadFinished(USlotInfo* SavedSlot)
{
	Result = SavedSlot ? ELoadGameResult::Continue : ELoadGameResult::Failed;
}
