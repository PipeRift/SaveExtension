// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "TestHelpers.h"

#include <Engine/Engine.h>
#include <Engine/LocalPlayer.h>
#include <GameFramework/Actor.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/WorldSettings.h>
#include <EngineUtils.h>


TArray<UWorld*> FSaveSpec::Worlds;

TArray<UWorld*> FSaveSpec::PendingWorldsToCleanup;


UWorld* FSaveSpec::GetPrimaryWorld() const
{
	UWorld* ReturnVal = nullptr;
	if (!GEngine)
	{
		for (auto It = GEngine->GetWorldContexts().CreateConstIterator(); It; ++It)
		{
			const FWorldContext& Context = *It;
			if ((Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE) && Context.World())
			{
				ReturnVal = Context.World();
				break;
			}
		}
	}
	return ReturnVal;
}

UWorld* FSaveSpec::CreateTestWorld()
{
	UWorld* ReturnVal = nullptr;

	// Unfortunately, this hack is needed, to avoid a crash when running as commandlet
	// NOTE: Sometimes, depending upon build settings, PRIVATE_GIsRunningCommandlet is a define instead of a global
#ifndef PRIVATE_GIsRunningCommandlet
	bool bIsCommandlet = PRIVATE_GIsRunningCommandlet;

	PRIVATE_GIsRunningCommandlet = false;
#endif

	ReturnVal = UWorld::CreateWorld(EWorldType::None, false);

#ifndef PRIVATE_GIsRunningCommandlet
	PRIVATE_GIsRunningCommandlet = bIsCommandlet;
#endif

	if (ReturnVal != nullptr)
	{
		Worlds.Add(ReturnVal);

		// Hack-mark the world as having begun play (when it has not)
		ReturnVal->bBegunPlay = true;

		// Hack-mark the world as having initialized actors (to allow RPC hooks)
		ReturnVal->bActorsInitialized = true;

		// Enable pause, using the PlayerController of the primary world (unless we're in the editor)
		// @todo #JohnB: Broken in the commandlet. No LocalPlayer
		if (!GIsEditor)
		{
			AWorldSettings* CurSettings = ReturnVal->GetWorldSettings();
			if (CurSettings)
			{
				ULocalPlayer* PrimLocPlayer = GEngine->GetFirstGamePlayer(GetPrimaryWorld());
				APlayerController* PrimPC = (PrimLocPlayer != nullptr ? PrimLocPlayer->PlayerController : nullptr);
				APlayerState * PrimState = (PrimPC != nullptr ? PrimPC->PlayerState : nullptr);

				if (PrimState)
				{
					CurSettings->SetPauserPlayerState(PrimState);
				}
			}
		}

		// Create a blank world context, to prevent crashes
		FWorldContext& CurContext = GEngine->CreateNewWorldContext(EWorldType::None);
		CurContext.SetCurrentWorld(ReturnVal);
	}

	return ReturnVal;
}

void FSaveSpec::DestroyTestWorld(UWorld* CleanupWorld)
{
	if (CleanupWorld)
	{
		Worlds.Remove(CleanupWorld);
		PendingWorldsToCleanup.Add(CleanupWorld);

		CleanupUnitTestWorlds();
	}
}

void FSaveSpec::CleanupUnitTestWorlds()
{
	for (auto CleanupIt = PendingWorldsToCleanup.CreateIterator(); CleanupIt; ++CleanupIt)
	{
		UWorld* CurWorld = *CleanupIt;

		// Iterate all ActorComponents in this world, and unmark them as having begun play - to prevent a crash during GC
		for (TActorIterator<AActor> ActorIt(CurWorld); ActorIt; ++ActorIt)
		{
			for (UActorComponent* CurComp : ActorIt->GetComponents())
			{
				if (CurComp->HasBegunPlay())
				{
					// Big hack - call only the parent class UActorComponent::EndPlay function, such that only bHasBegunPlay is unset
					bool bBeginDestroyed = CurComp->HasAnyFlags(RF_BeginDestroyed);

					CurComp->SetFlags(RF_BeginDestroyed);

					CurComp->UActorComponent::EndPlay(EEndPlayReason::Quit);

					if (!bBeginDestroyed)
					{
						CurComp->ClearFlags(RF_BeginDestroyed);
					}
				}
			}
		}

		GEngine->DestroyWorldContext(CurWorld);
		CurWorld->DestroyWorld(false);
	}

	PendingWorldsToCleanup.Empty();


	// Immediately garbage collect remaining objects, to finish net driver cleanup
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, true);
}
