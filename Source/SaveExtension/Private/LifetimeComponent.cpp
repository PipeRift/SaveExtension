// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "LifetimeComponent.h"
#include "SlotDataTask.h"


ULifetimeComponent::ULifetimeComponent()
	: Super()
{}

void ULifetimeComponent::BeginPlay()
{
	Super::BeginPlay();

	if (USaveManager* Manager = GetManager())
	{
		Manager->SubscribeForEvents(this);

		// If manager is loading, it has probably manually created
		// this actor and its not a natural start
		if (!Manager->IsLoading())
		{
			Start.Broadcast();
		}
	}
	else
	{
		UE_LOG(LogSaveExtension, Error, TEXT("LifetimeComponent couldnt find a SaveManager. It will do nothing."))
	}
}

void ULifetimeComponent::EndPlay(EEndPlayReason::Type Reason)
{
	if (USaveManager* Manager = GetManager())
	{
		// If manager is loading, it has probably manually destroyed
		// this actor and its not a natural destroy
		if (!Manager->IsLoading())
		{
			Finish.Broadcast();
		}
	}

	Super::EndPlay(Reason);
}

void ULifetimeComponent::OnSaveBegan()
{
	if (USlotDataTask::ShouldSave(GetOwner()))
	{
		Saved.Broadcast();
	}
}

void ULifetimeComponent::OnLoadFinished(bool bError)
{
	if (USlotDataTask::ShouldSave(GetOwner()))
	{
		Resume.Broadcast();
	}
}
