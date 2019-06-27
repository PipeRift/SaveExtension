// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "Serialization/MTTask_SerializeActors.h"

#include <Components/CapsuleComponent.h>
#include <Engine/LocalPlayer.h>
#include <GameFramework/GameModeBase.h>
#include <GameFramework/GameStateBase.h>
#include <GameFramework/HUD.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>
#include <Kismet/GameplayStatics.h>
#include <Serialization/MemoryWriter.h>

#include "SaveManager.h"
#include "SlotInfo.h"
#include "SlotData.h"
#include "SavePreset.h"


/////////////////////////////////////////////////////
// FMTTask_SerializeActors
void FMTTask_SerializeActors::DoWork()
{
	bool bIsAIController;
	bool bIsLevelScript;

	if (bStoreMainActors)
	{
		if (bStoreGameInstance)
			SerializeGameInstance();

		if (bStoreGameMode)
		{
			SerializeGameMode();
			SerializeGameState();

			if (World->GetGameInstance()->GetLocalPlayers().Num() > 0)
			{
				const int32 PlayerId = 0;

				SerializePlayerState(PlayerId);
				SerializePlayerController(PlayerId);
				//SerializePlayerPawn(PlayerId);
				SerializePlayerHUD(PlayerId);
			}
		}
	}

	for (int32 I = 0; I < Num; ++I)
	{
		const AActor* const Actor = (*LevelActors)[StartIndex + I];
		if (ShouldSave(Actor))
		{
			bIsAIController = false;
			bIsLevelScript = false;

			if (ShouldSaveAsWorld(Actor, bIsAIController, bIsLevelScript))
			{
				if (bIsAIController)
				{
					if (const AAIController* const AI = Cast<AAIController>(Actor))
					{
						FControllerRecord Record;
						SerializeController(AI, Record);
						AIControllerRecords.Add(MoveTemp(Record));
					}
				}
				else if (bIsLevelScript)
				{
					if (const ALevelScriptActor* const LevelScript = Cast<ALevelScriptActor>(Actor))
					{
						SerializeActor(LevelScript, LevelScriptRecord);
					}
				}
				else
				{
					FActorRecord Record;
					SerializeActor(Actor, Record);
					ActorRecords.Add(MoveTemp(Record));
				}
			}
		}
	}
}

bool FMTTask_SerializeActors::SerializeController(const AController* Actor, FControllerRecord& Record) const
{
	const bool bResult = SerializeActor(Actor, Record);
	if (bResult && bStoreControlRotation)
	{
		Record.ControlRotation = Actor->GetControlRotation();
	}
	return bResult;
}

void FMTTask_SerializeActors::SerializeGameMode()
{
	if (ShouldSave(World->GetAuthGameMode()))
	{
		SerializeActor(World->GetAuthGameMode(), SlotData->GameMode);
	}
}

void FMTTask_SerializeActors::SerializeGameState()
{
	const auto* GameState = World->GetGameState();
	if (ShouldSave(GameState))
	{
		SerializeActor(GameState, SlotData->GameState);
	}
}

void FMTTask_SerializeActors::SerializePlayerState(int32 PlayerId)
{
	const auto* Controller = UGameplayStatics::GetPlayerController(World, PlayerId);
	if (Controller && ShouldSave(Controller->PlayerState))
	{
		SerializeActor(Controller->PlayerState, SlotData->PlayerState);
	}
}

void FMTTask_SerializeActors::SerializePlayerController(int32 PlayerId)
{
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, PlayerId);
	if (ShouldSave(PlayerController))
	{
		SerializeController(PlayerController, SlotData->PlayerController);
	}
}

void FMTTask_SerializeActors::SerializePlayerHUD(int32 PlayerId)
{
	const auto* Controller = UGameplayStatics::GetPlayerController(World, PlayerId);
	if (Controller && ShouldSave(Controller->MyHUD))
	{
		SerializeActor(Controller->MyHUD, SlotData->PlayerHUD);
	}
}

void FMTTask_SerializeActors::SerializeGameInstance()
{
	UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance)
	{
		FObjectRecord Record{ GameInstance };

		//Serialize into Record Data
		FMemoryWriter MemoryWriter(Record.Data, true);
		FSaveExtensionArchive Archive(MemoryWriter, false);
		GameInstance->Serialize(Archive);

		SlotData->GameInstance = MoveTemp(Record);
	}
}

bool FMTTask_SerializeActors::SerializeActor(const AActor* Actor, FActorRecord& Record) const
{

	//Clean the record
	Record = { Actor };

	Record.bHiddenInGame = Actor->bHidden;
	Record.bIsProcedural = IsProcedural(Actor);

	if (SavesTags(Actor))
	{
		Record.Tags = Actor->Tags;
	}
	else
	{
		// Only save save-tags
		for (const auto& Tag : Actor->Tags)
		{
			if (USlotDataTask::IsSaveTag(Tag))
			{
				Record.Tags.Add(Tag);
			}
		}
	}

	if (SavesTransform(Actor))
	{
		Record.Transform = Actor->GetTransform();

		if (SavesPhysics(Actor))
		{
			USceneComponent* const Root = Actor->GetRootComponent();
			if (Root && Root->Mobility == EComponentMobility::Movable)
			{
				if (auto* const Primitive = Cast<UPrimitiveComponent>(Root))
				{
					Record.LinearVelocity = Primitive->GetPhysicsLinearVelocity();
					Record.AngularVelocity = Primitive->GetPhysicsAngularVelocityInRadians();
				}
				else
				{
					Record.LinearVelocity = Root->GetComponentVelocity();
				}
			}
		}
	}

	if (SavesComponents(Actor))
	{
		SerializeActorComponents(Actor, Record, 1);
	}

	FMemoryWriter MemoryWriter(Record.Data, true);
	FSaveExtensionArchive Archive(MemoryWriter, false);
	const_cast<AActor*>(Actor)->Serialize(Archive);

	return true;
}

void FMTTask_SerializeActors::SerializeActorComponents(const AActor* Actor, FActorRecord& ActorRecord, int8 Indent /*= 0*/) const
{
	const TSet<UActorComponent*>& Components = Actor->GetComponents();
	for (auto* Component : Components)
	{
		if (ShouldSave(Component))
		{
			FComponentRecord ComponentRecord;
			ComponentRecord.Name = Component->GetFName();
			ComponentRecord.Class = Component->GetClass();

			if (SavesTransform(Component))
			{
				const USceneComponent* Scene = CastChecked<USceneComponent>(Component);
				if (Scene->Mobility == EComponentMobility::Movable)
				{
					ComponentRecord.Transform = Scene->GetRelativeTransform();
				}
			}

			if (SavesTags(Component))
			{
				ComponentRecord.Tags = Component->ComponentTags;
			}

			if (!Component->GetClass()->IsChildOf<UPrimitiveComponent>())
			{
				FMemoryWriter MemoryWriter(ComponentRecord.Data, true);
				FSaveExtensionArchive Archive(MemoryWriter, false);
				Component->Serialize(Archive);
			}
			ActorRecord.ComponentRecords.Add(ComponentRecord);
		}
	}
}
