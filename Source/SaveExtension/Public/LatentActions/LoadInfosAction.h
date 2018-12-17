// Copyright 2015-2018 Piperift. All Rights Reserved.

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
enum class ELoadInfoResult : uint8
{
	Completed
};

/** FLoadInfosction
 * A say quote action; counts down and triggers it's output link when the time remaining falls to zero
 */
class FLoadInfosAction : public FPendingLatentAction
{

public:
	ELoadInfoResult& Result;

	TArray<USlotInfo*>& SlotInfos;
	bool bFinished;

	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;


	/**
	 * @param SlotId will load that Saved Game if Id > 0, otherwise it will load all infos
	 */
	FLoadInfosAction(USaveManager* Manager, const bool bSortByRecent, TArray<USlotInfo*>& SaveInfos, ELoadInfoResult& OutResult, const FLatentActionInfo& LatentInfo);

	virtual void UpdateOperation(FLatentResponse& Response) override;

#if WITH_EDITOR
	// Returns a human readable description of the latent operation's current state
	/*virtual FString GetDescription() const override
	{
		return FString::Printf(TEXT("Loading Info %i of %i"), loadedInfos, infosCount);
	}*/
#endif
};
