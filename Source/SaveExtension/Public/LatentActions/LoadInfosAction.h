// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "Multithreading/LoadSlotInfosTask.h"

#include <CoreMinimal.h>
#include <Engine/LatentActionManager.h>
#include <LatentActions.h>


class USaveManager;
class USaveSlot;

/**
 * Enum used to indicate quote execution results
 */
UENUM()
enum class ELoadInfoResult : uint8
{
	Completed
};

/** FLoadInfosction */
class FLoadInfosAction : public FPendingLatentAction
{
public:
	ELoadInfoResult& Result;

	TArray<USaveSlot*>& SlotInfos;
	bool bFinished;

	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;


	/**
	 * @param SlotId will load that Saved Game if Id > 0, otherwise it will load all infos
	 */
	FLoadInfosAction(USaveManager* Manager, const bool bSortByRecent, TArray<USaveSlot*>& SaveInfos,
		ELoadInfoResult& OutResult, const FLatentActionInfo& LatentInfo);

	virtual void UpdateOperation(FLatentResponse& Response) override;

#if WITH_EDITOR
	virtual FString GetDescription() const override
	{
		return TEXT("Loading all infos...");
	}
#endif
};
