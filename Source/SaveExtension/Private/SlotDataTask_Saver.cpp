// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "SlotDataTask_Saver.h"

#include <Kismet/GameplayStatics.h>
#include <Engine/LocalPlayer.h>
#include <GameFramework/GameModeBase.h>
#include <GameFramework/GameStateBase.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/HUD.h>
#include <Serialization/MemoryWriter.h>
#include <Components/CapsuleComponent.h>

#include "SaveManager.h"
#include "SlotInfo.h"
#include "SlotData.h"
#include "SavePreset.h"
#include "FileAdapter.h"


/////////////////////////////////////////////////////
// USaveDataTask_Saver

void USlotDataTask_Saver::OnStart()
{
	USaveManager* Manager = GetManager();
	Manager->TryInstantiateInfo();

	bool bSave = true;
	const FString InfoCard = Manager->GenerateSaveSlotName(Slot);
	const FString DataCard = Manager->GenerateSaveDataSlotName(Slot);

	//Overriding
	{
		const bool bInfoExists = FFileAdapter::DoesFileExist(InfoCard);
		const bool bDataExists = FFileAdapter::DoesFileExist(DataCard);

		if (bOverride)
		{
			// Delete previous save
			if (bInfoExists)
			{
				FFileAdapter::DeleteFile(InfoCard);
			}
			if (bDataExists)
			{
				FFileAdapter::DeleteFile(DataCard);
			}
		}
		else
		{
			//Only save if previous files don't exist
			//We don't want to serialize since it won't be saved anyway
			bSave = !bInfoExists && !bDataExists;
		}
	}

	if (bSave)
	{
		USlotInfo* CurrentInfo = Manager->GetCurrentInfo();
		SlotData = Manager->GetCurrentData();
		SlotData->Clean(false);


		check(CurrentInfo && SlotData);

		const bool bSlotWasDifferent = CurrentInfo->Id != Slot;
		CurrentInfo->Id = Slot;

		if (bSaveThumbnail)
		{
			CurrentInfo->SaveThumbnail(Width, Height);
		}

		// Time stats
		{
			CurrentInfo->SaveDate = FDateTime::Now();

			// If this info has been loaded ever
			const bool bWasLoaded = CurrentInfo->LoadDate.GetTicks() > 0;
			if (bWasLoaded)
			{
				// Now - Loaded
				const FTimespan SessionTime = CurrentInfo->SaveDate - CurrentInfo->LoadDate;

				CurrentInfo->TotalPlayedTime += SessionTime;

				if (!bSlotWasDifferent)
					CurrentInfo->SlotPlayedTime += SessionTime;
				else
					CurrentInfo->SlotPlayedTime = SessionTime;
			}
			else
			{
				// Slot is new, played time is world seconds
				CurrentInfo->TotalPlayedTime = FTimespan::FromSeconds(World->TimeSeconds);
				CurrentInfo->SlotPlayedTime = CurrentInfo->TotalPlayedTime;
			}
		}

		//Save Level info in both files
		CurrentInfo->Map = World->GetFName();
		SlotData->Map = World->GetFName().ToString();

		SerializeSync();
		SaveFile(InfoCard, DataCard);

		// Clean serialization data
		SlotData->Clean(true);

		SE_LOG(Preset, "Finished Saving", FColor::Green);
	}
	Finish(bSave);
}

void USlotDataTask_Saver::SerializeSync()
{
	// Save World
	SerializeWorld();

	if (Preset->bStoreGameInstance)
		SerializeGameInstance();

	if (Preset->bStoreGameMode)
	{
		SerializeGameMode();
		SerializeGameState();

		const TArray<ULocalPlayer*>& LocalPlayers = World->GetGameInstance()->GetLocalPlayers();
		for (const auto* LocalPlayer : LocalPlayers)
		{
			int32 PlayerId = LocalPlayer->GetControllerId();

			SerializePlayerState(PlayerId);
			SerializePlayerController(PlayerId);
			//SerializePlayerPawn(PlayerId);
			SerializePlayerHUD(PlayerId);
		}
	}
}

void USlotDataTask_Saver::SerializeWorld()
{
	check(World);
	SE_LOG(Preset, "World '" + World->GetName() + "'", FColor::Green, false, 1);

	// Save current game seconds
	SlotData->TimeSeconds = World->TimeSeconds;

	SerializeLevelSync(World->GetCurrentLevel());

	const TArray<ULevelStreaming*>& Levels = World->GetStreamingLevels();
	for (const ULevelStreaming* Level : Levels)
	{
		if (Level->IsLevelLoaded())
		{
			SerializeLevelSync(Level->GetLoadedLevel(), Level);
		}
	}
}

void USlotDataTask_Saver::SerializeLevelSync(const ULevel* Level, const ULevelStreaming* StreamingLevel)
{
	check(IsValid(Level));

	const FName LevelName = StreamingLevel ? StreamingLevel->GetWorldAssetPackageFName() : FPersistentLevelRecord::PersistentName;
	SE_LOG(Preset, "Level '" + LevelName.ToString() + "'", FColor::Green, false, 1);

	FLevelRecord* LevelRecord = &SlotData->MainLevel;

	if (StreamingLevel)
	{
		// Find or create the sub-level
		int32 Index = SlotData->SubLevels.IndexOfByKey(StreamingLevel);
		if (Index == INDEX_NONE)
		{
			Index = SlotData->SubLevels.Add({ StreamingLevel });
		}
		LevelRecord = &SlotData->SubLevels[Index];
	}
	check(LevelRecord);

	// Empty level before we serialize
	LevelRecord->Clean();


	//All actors of a class
	for (auto ActorItr = Level->Actors.CreateConstIterator(); ActorItr; ++ActorItr)
	{
		const AActor* Actor = *ActorItr;

		if (ShouldSaveAsWorld(Actor))
		{
			const UClass* ActorClass = Actor->GetClass();

			if (const AAIController* AI = Cast<AAIController>(Actor))
			{
				SerializeAI(AI, *LevelRecord);
			}
			else if (const ALevelScriptActor* LevelScript = Cast<ALevelScriptActor>(Actor))
			{
				SerializeLevelScript(LevelScript, *LevelRecord);
			}
			else
			{
				FActorRecord Record;
				SerializeActor(Actor, Record);
				LevelRecord->Actors.Add(Record);

				//SE_LOG(Preset, "Actor '" + Actor->GetName() + "'", FColor::White, false, 2);
			}
		}
	}
}

void USlotDataTask_Saver::SerializeLevelScript(const ALevelScriptActor* Level, FLevelRecord& LevelRecord)
{
	if (Preset->bStoreLevelBlueprints)
	{
		check(Level);

		if (Level->IsInPersistentLevel())
		{
			SerializeActor(Level, LevelRecord.LevelScript);
		}
		else
		{
			//TODO: Serialize into its own streaming level
		}

		SE_LOG(Preset, "Level Blueprint '" + Level->GetName() + "'", FColor::White, false, 2);
	}
}

void USlotDataTask_Saver::SerializeAI(const AAIController* AIController, FLevelRecord& LevelRecord)
{
	if (Preset->bStoreAIControllers)
	{
		check(AIController);

		FControllerRecord Record;
		SerializeController(AIController, Record);
		LevelRecord.AIControllers.Add(Record);

		SE_LOG(Preset, "AI Controller '" + AIController->GetName() + "'", FColor::White, 1);
	}
}

void USlotDataTask_Saver::SerializeGameMode()
{
	FActorRecord& Record = SlotData->GameMode;
	const bool bSuccess = SerializeActor(World->GetAuthGameMode(), Record);

	SE_LOG(Preset, "Game Mode '" + Record.Name.ToString() + "'", FColor::Green, !bSuccess, 1);
}

void USlotDataTask_Saver::SerializeGameState()
{
	const auto* GameState = World->GetGameState();

	FActorRecord& Record = SlotData->GameState;
	SerializeActor(GameState, Record);

	SE_LOG(Preset, "Game State '" + Record.Name.ToString() + "'", 1);
}

void USlotDataTask_Saver::SerializePlayerState(int32 PlayerId)
{
	const auto* Controller = UGameplayStatics::GetPlayerController(World, PlayerId);
	if (Controller)
	{
		FActorRecord& Record = SlotData->PlayerState;
		const bool bSuccess = SerializeActor(Controller->PlayerState, Record);

		SE_LOG(Preset, "Player State '" + Record.Name.ToString() + "'", FColor::Green, !bSuccess, 1);
	}
}

void USlotDataTask_Saver::SerializePlayerController(int32 PlayerId)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, PlayerId);

	FControllerRecord& Record = SlotData->PlayerController;
	const bool bSuccess = SerializeController(PlayerController, Record);

	SE_LOG(Preset, "Player Controller '" + Record.Name.ToString() + "'", FColor::Green, !bSuccess, 1);
}

void USlotDataTask_Saver::SerializePlayerHUD(int32 PlayerId)
{
	const auto* Controller = UGameplayStatics::GetPlayerController(World, PlayerId);
	if (Controller)
	{
		FActorRecord& Record = SlotData->PlayerHUD;
		const bool bSuccess = SerializeActor(Controller->MyHUD, Record);

		SE_LOG(Preset, "Player HUD '" + Record.Name.ToString() + "'", FColor::Green, !bSuccess, 1);
	}
}

void USlotDataTask_Saver::SerializeGameInstance()
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(World);
	if (!GameInstance)
	{
		SE_LOG(Preset, "Game Instance - No Game Instance Found", FColor::White, true, 1);
		return;
	}

	FObjectRecord Record { GameInstance };

	//Serialize into Record Data
	FMemoryWriter MemoryWriter(Record.Data, true);
	FSaveExtensionArchive Archive(MemoryWriter, false);
	GameInstance->Serialize(Archive);

	SlotData->GameInstance = MoveTemp(Record);

	SE_LOG(Preset, "Game Instance '" + Record.Name.ToString() + "'", FColor::White, 1);
}

bool USlotDataTask_Saver::SerializeActor(const AActor* Actor, FActorRecord& Record)
{
	if (!ShouldSave(Actor))
		return false;

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
			if (IsSaveTag(Tag))
			{
				Record.Tags.Add(Tag);
			}
		}
	}

	const bool bSavesPhysics = SavesPhysics(Actor);
	if (SavesTransform(Actor) || bSavesPhysics)
	{
		Record.Transform = Actor->GetTransform();
	}

	if (bSavesPhysics)
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

	if (SavesComponents(Actor))
	{
		SerializeActorComponents(Actor, Record, 1);
	}

	FMemoryWriter MemoryWriter(Record.Data, true);
	FSaveExtensionArchive Archive(MemoryWriter, false);
	const_cast<AActor*>(Actor)->Serialize(Archive);

	return true;
}

bool USlotDataTask_Saver::SerializeController(const AController* Actor, FControllerRecord& Record)
{
	if (ShouldSave(Actor))
	{
		const bool bResult = SerializeActor(Actor, Record);
		if (bResult && Preset->bStoreControlRotation)
		{
			Record.ControlRotation = Actor->GetControlRotation();
		}
		return bResult;
	}
	return false;
}

void USlotDataTask_Saver::SerializeActorComponents(const AActor* Actor, FActorRecord& ActorRecord, int8 Indent)
{
	const TSet<UActorComponent*>& Components = Actor->GetComponents();
	for (auto* Component : Components)
	{
		if (!IsValid(Component) ||
			!ShouldSave(Component))
		{
			continue;
		}

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

bool USlotDataTask_Saver::SaveFile(const FString& InfoName, const FString& DataName) const
{
	USaveManager* Manager = GetManager();
	const USavePreset* Preset = Manager->GetPreset();

	USlotInfo* CurrentInfo = Manager->GetCurrentInfo();
	USlotData* CurrentData = Manager->GetCurrentData();

	if (FFileAdapter::SaveFile(CurrentInfo, InfoName, Preset) &&
		FFileAdapter::SaveFile(CurrentData, DataName, Preset))
	{
		return true;
	}

	return false;
}
