// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Engine/LatentActionManager.h>
#include <LatentActions.h>


class USaveManager;
class USlotInfo;

/**
 * Enum used to indicate quote execution results
 */
UENUM()
enum class ELoadGameResult : uint8
{
	Loading UMETA(Hidden),
	Continue,
	Failed
};

/** FLoadGameAction */
class FLoadGameAction : public FPendingLatentAction
{
public:
	ELoadGameResult& Result;

	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;


	FLoadGameAction(USaveManager* Manager, FName SlotName, ELoadGameResult& Result, const FLatentActionInfo& LatentInfo);

	virtual void UpdateOperation(FLatentResponse& Response) override;

	void OnLoadFinished(USlotInfo* SavedSlot);

#if WITH_EDITOR
	// Returns a human readable description of the latent operation's current state
	virtual FString GetDescription() const override
	{
		return TEXT("Loading Game...");
	}
#endif
};
