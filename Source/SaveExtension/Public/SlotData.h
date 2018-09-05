// Copyright 2015-2018 Piperift. All Rights Reserved.


#pragma once

#include "ISaveExtension.h"

 #include <CoreMinimal.h>
#include <Engine/LevelStreaming.h>
#include <Engine/LevelScriptActor.h>
#include <GameFramework/SaveGame.h>
#include <Serialization/ObjectAndNameAsStringProxyArchive.h>

#include "SlotData.generated.h"

class USlotData;


USTRUCT()
struct FBaseRecord {
	GENERATED_USTRUCT_BODY()

	FBaseRecord()
		: Name{}
		, Class(nullptr)
	{}

	UPROPERTY(SaveGame)
	FName Name;

	UPROPERTY(SaveGame)
	UClass* Class;
};

FORCEINLINE bool operator==(FBaseRecord&&      A, FBaseRecord&&      B) { return A.Name == B.Name; }
FORCEINLINE bool operator==(const FBaseRecord& A, const FBaseRecord& B) { return A.Name == B.Name; }


/**
* Represents a serialized Object
*/
USTRUCT()
struct FObjectRecord : public FBaseRecord {
	GENERATED_USTRUCT_BODY()

	FObjectRecord() : Super() {}
	FObjectRecord(const UObject* Object)
		: Super()
	{
		if (Object)
		{
			Name = Object->GetFName();
			Class = Object->GetClass();
		}
	}

	UPROPERTY(SaveGame)
	TArray<uint8> Data;

	UPROPERTY(SaveGame)
	TArray<FName> Tags;


	bool IsValid() const {
		return !Name.IsNone() && Class && Data.Num() > 0;
	}

	FORCEINLINE bool operator== (const UObject* Other) const { return Name == Other->GetFName() && Class == Other->GetClass(); }
};

/**
* Represents a serialized Component
*/
USTRUCT()
struct FComponentRecord : public FObjectRecord {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(SaveGame)
	FTransform Transform;
};

/**
* Represents a serialized Actor
*/
USTRUCT()
struct FActorRecord : public FObjectRecord {
	GENERATED_USTRUCT_BODY()

	FActorRecord() : Super() {}
	FActorRecord(const AActor* Actor)
		: Super(Actor)
	{}


	UPROPERTY(SaveGame)
	FString Level;

	UPROPERTY(SaveGame)
	bool bHiddenInGame;

	/** Whether or not this actor was spawned in runtime */
	UPROPERTY(SaveGame)
	bool bIsProcedural;

	UPROPERTY(SaveGame)
	FTransform Transform;

	UPROPERTY(SaveGame)
	FVector LinearVelocity;

	UPROPERTY(SaveGame)
	FVector AngularVelocity;

	UPROPERTY(SaveGame)
	TArray<FComponentRecord> ComponentRecords;
};

/**
* Represents a serialized Controller
*/
USTRUCT()
struct FControllerRecord : public FActorRecord {
	GENERATED_USTRUCT_BODY()

	FControllerRecord() = default;

	UPROPERTY(SaveGame)
	FRotator ControlRotation;
};

/**
* Represents a destroyed object
*/
USTRUCT()
struct FDestroyedActorRecord : public FBaseRecord {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(SaveGame)
	FString Level;

	bool IsValid() const {
		return !Name.IsNone() && Class && !Level.IsEmpty();
	}

	FORCEINLINE bool operator== (const AActor* Other) const { return Name == Other->GetFName() && Class == Other->GetClass(); }
};

/**
* Represents a level in the world (streaming or persistent)
*/
USTRUCT()
struct FLevelRecord : public FBaseRecord {
	GENERATED_USTRUCT_BODY()

	FLevelRecord() : Super() {}

	/** Record of the Level Script Actor */
	UPROPERTY()
	FActorRecord LevelScript;

	/** Records of the World Actors */
	UPROPERTY()
	TArray<FActorRecord> Actors;

	/** Records of the AI Controller Actors */
	UPROPERTY()
	TArray<FControllerRecord> AIControllers;

	bool IsValid() const {
		return !Name.IsNone();
	}

	void Clean();
};

/** Represents a persistent level in the world */
USTRUCT()
struct FPersistentLevelRecord : public FLevelRecord {
	GENERATED_USTRUCT_BODY()

	static FName PersistentName;

	FPersistentLevelRecord() : Super() {
		Name = PersistentName;
	}
};

/** Represents an streaming level in the world */
USTRUCT()
struct FStreamingLevelRecord : public FLevelRecord {
	GENERATED_USTRUCT_BODY()

	FStreamingLevelRecord() : Super() {}
	FStreamingLevelRecord(const ULevelStreaming* Level)
		: Super()
	{
		Class = ULevelStreaming::StaticClass();
		if (Level)
			Name = Level->GetWorldAssetPackageFName();
	}

	FORCEINLINE bool operator== (const ULevelStreaming* Level) const {
		return Level && Name == Level->GetWorldAssetPackageFName();
	}
};


/**
 * In charge of serializing SaveGame data
 */
struct FSaveExtensionArchive : public FObjectAndNameAsStringProxyArchive
{
	FSaveExtensionArchive(FArchive &InInnerArchive, bool bInLoadIfFindFails)
		: FObjectAndNameAsStringProxyArchive(InInnerArchive,bInLoadIfFindFails)
	{
		ArIsSaveGame = true;
		ArNoDelta = true;
	}

	// Courtesy of Colin Bonstead
	SAVEEXTENSION_API FArchive& operator<<(struct FSoftObjectPtr& Value);
	SAVEEXTENSION_API FArchive& operator<<(struct FSoftObjectPath& Value);
};


/**
* USaveData is the save game object in charge of saving all heavy info about a saved game.
* E.g: Quests, Enemies, World Actors, Physics
*/
UCLASS(ClassGroup = SaveExtension, hideCategories = ("Activation", "Actor Tick", "Actor", "Input", "Rendering", "Replication", "Socket", "Thumbnail"))
class SAVEEXTENSION_API USlotData : public USaveGame
{
	GENERATED_UCLASS_BODY()

public:

	/** If Game is multiplayer, SaveGame system generates SaveGame Data for each player. */
	UPROPERTY(Category = SaveData, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "63"))
	int32 PlayerId;

	/** Full Name of the Map where this SlotData was saved */
	UPROPERTY(Category = SaveData, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FString Map;

	/** Game world time since game started in seconds */
	UPROPERTY(Category = SaveData, BlueprintReadOnly)
	float TimeSeconds;


	/** Records
	 * All serialized information to be saved or loaded
	 */

	UPROPERTY()
	FObjectRecord GameInstance;
	UPROPERTY()
	FActorRecord GameMode;
	UPROPERTY()
	FActorRecord GameState;

	UPROPERTY()
	FActorRecord PlayerPawn;
	UPROPERTY()
	FControllerRecord PlayerController;
	UPROPERTY()
	FActorRecord PlayerState;
	UPROPERTY()
	FActorRecord PlayerHUD;

	UPROPERTY(SaveGame)
	FPersistentLevelRecord MainLevel;
	UPROPERTY(SaveGame)
	TArray<FStreamingLevelRecord> SubLevels;


	void Clean(bool bKeepLevels);
	FName GetFMap() const { return { *Map }; }
};
