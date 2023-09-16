// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "LatentActions/SaveGameAction.h"

#include "SaveManager.h"
#include "SaveSlot.h"


FSaveGameAction::FSaveGameAction(USaveManager* Manager, FName SlotName, bool bOverrideIfNeeded,
	bool bScreenshot, const FScreenshotSize Size, ESaveGameResult& OutResult,
	const FLatentActionInfo& LatentInfo)
	: Result(OutResult)
	, ExecutionFunction(LatentInfo.ExecutionFunction)
	, OutputLink(LatentInfo.Linkage)
	, CallbackTarget(LatentInfo.CallbackTarget)
{
	const bool bStarted = Manager->SaveSlot(SlotName, bOverrideIfNeeded, bScreenshot, Size,
		FOnGameSaved::CreateRaw(this, &FSaveGameAction::OnSaveFinished));

	if (!bStarted)
	{
		Result = ESaveGameResult::Failed;
	}
}

void FSaveGameAction::UpdateOperation(FLatentResponse& Response)
{
	Response.FinishAndTriggerIf(
		Result != ESaveGameResult::Saving, ExecutionFunction, OutputLink, CallbackTarget);
}

void FSaveGameAction::OnSaveFinished(USaveSlot* SavedSlot)
{
	Result = SavedSlot ? ESaveGameResult::Continue : ESaveGameResult::Failed;
}
