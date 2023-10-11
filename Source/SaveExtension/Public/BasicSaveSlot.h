// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "SaveSlot.h"

#include <Components/ActorComponent.h>
#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include <GameFramework/SaveGame.h>

#include "BasicSaveSlot.generated.h"


struct FSELevelFilter;
class UTexture2D;


/**
 * USaveSlot stores information that needs to be accessible WITHOUT loading the game.
 * Works like a common SaveGame object
 * E.g: Dates, played time, progress, level
 */
UCLASS()
class SAVEEXTENSION_API UBasicSaveSlot : public USaveSlot
{
	GENERATED_BODY()

	void StartSerialization() {}
	void EndSerialization() {}

	template <typename T>
	void RunTask()
	{}

	void SaveMemoryToFile() {}
	void LoadMemoryFromFile() {}


	/* Psudo code for manual syntax
	void LoadAll()
	{
		LoadMemoryFromFile(EFileMode::Async, []()
		{
			LoadActors(WorldFilter, ELoadMode::Parallel);
			if (bRespawningAfterKilled)	   // Dont restore AI if player was killed
			{
				LoadActors(AIFilter, ELoadMode::GameThread);
			}
			LoadPlayers();
			NotifyLoad(ELoadType::Full);
		});
	}

	void SaveAll()
	{
		ClearMemory();
		SavePlayers();
		if (bRespawningAfterKilled)	   // Dont restore AI if player was killed
		{
			SaveActors(AIFilter, ELoadMode::GameThread);
		}
		SaveActors(WorldFilter, ELoadMode::Parallel);
		CaptureThumbnail(Resolution, []()
		{
			SaveMemoryToFile(EFileMode::Async);
		});
		NotifySave(ESaveType::Full);
	}

	void OnPostLevelLoad(ULevel* Level)
	{
		LoadActors(WorldFilter, Level);
		LoadActors(AIFilter, Level);
		NotifyLoad(ELoadType::Level);
	}

	void OnPreLevelSave(ULevel* Level)
	{
		SaveActors(AIFilter, Level);
		SaveActors(WorldFilter, Level);
		NotifySave(ESaveType::Level);
	}*/
};
