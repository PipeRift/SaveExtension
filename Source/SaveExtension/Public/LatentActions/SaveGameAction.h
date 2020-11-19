// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Engine/LatentActionManager.h>
#include <LatentActions.h>


class USaveManager;
class USlotInfo;
struct FScreenshotSize;

/**
 * Enum used to indicate quote execution results
 */
UENUM()
enum class ESaveGameResult : uint8
{
	Saving UMETA(Hidden),
	Continue,
	Failed
};

/** FSaveGameAction */
class FSaveGameAction : public FPendingLatentAction
{

public:
	ESaveGameResult& Result;

	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;


	FSaveGameAction(USaveManager* Manager, FName SlotName, bool bOverrideIfNeeded, bool bScreenshot, const FScreenshotSize Size, ESaveGameResult& OutResult, const FLatentActionInfo& LatentInfo);

	virtual void UpdateOperation(FLatentResponse& Response) override;

	void OnSaveFinished(USlotInfo* SavedSlot);

#if WITH_EDITOR
	// Returns a human readable description of the latent operation's current state
	virtual FString GetDescription() const override
	{
		return TEXT("Saving Game...");
	}
#endif
};
