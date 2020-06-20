// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Engine/LatentActionManager.h>
#include <LatentActions.h>

#include "Multithreading/LoadAllSlotInfosTask.h"


class USaveManager;
class USlotInfo;

/**
 * Enum used to indicate quote execution results
 */
UENUM()
enum class EDeleteSlotsResult : uint8
{
	Completed
};

/** FLoadInfosction */
class FDeleteSlotsAction : public FPendingLatentAction
{
public:
	EDeleteSlotsResult& Result;

	bool bFinished;

	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;


	/**
	 * @param SlotId will load that Saved Game if Id > 0, otherwise it will load all infos
	 */
	FDeleteSlotsAction(USaveManager* Manager, EDeleteSlotsResult& OutResult, const FLatentActionInfo& LatentInfo);

	virtual void UpdateOperation(FLatentResponse& Response) override;

#if WITH_EDITOR
	// Returns a human readable description of the latent operation's current state
	virtual FString GetDescription() const override
	{
		return TEXT("Deleting all slots...");
	}
#endif
};
