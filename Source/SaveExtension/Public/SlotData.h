// Copyright 2015-2019 Piperift. All Rights Reserved.


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

	FName Name;
	TWeakObjectPtr<UClass> Class;


	FBaseRecord() : Name(), Class(nullptr) {}

	virtual bool Serialize(FArchive& Ar);
	friend FArchive& operator<<(FArchive& Ar, FBaseRecord& Record)
	{
		Record.Serialize(Ar);
		return Ar;
	}
	virtual ~FBaseRecord() {}
};

template<>
struct TStructOpsTypeTraits<FBaseRecord> : public TStructOpsTypeTraitsBase2<FBaseRecord>
{ enum { WithSerializer = true }; };

FORCEINLINE bool operator==(FBaseRecord&&      A, FBaseRecord&&      B) { return A.Name == B.Name; }
FORCEINLINE bool operator==(const FBaseRecord& A, const FBaseRecord& B) { return A.Name == B.Name; }


/** Represents a serialized Object */
USTRUCT()
struct FObjectRecord : public FBaseRecord {
	GENERATED_USTRUCT_BODY()

	TArray<uint8> Data;
	TArray<FName> Tags;


	FObjectRecord() : Super() {}
	FObjectRecord(const UObject* Object);

	virtual bool Serialize(FArchive& Ar) override;

	bool IsValid() const {
		return !Name.IsNone() && Class.IsValid() && Data.Num() > 0;
	}

	FORCEINLINE bool operator== (const UObject* Other) const { return Name == Other->GetFName() && Class == Other->GetClass(); }
};


/** Represents a serialized Component */
USTRUCT()
struct FComponentRecord : public FObjectRecord {
	GENERATED_USTRUCT_BODY()

	FTransform Transform;

	virtual bool Serialize(FArchive& Ar) override;
};


/** Represents a serialized Actor */
USTRUCT()
struct FActorRecord : public FObjectRecord {
	GENERATED_USTRUCT_BODY()


	bool bHiddenInGame;
	/** Whether or not this actor was spawned in runtime */
	bool bIsProcedural;
	FTransform Transform;
	FVector LinearVelocity;
	FVector AngularVelocity;
	TArray<FComponentRecord> ComponentRecords;


	FActorRecord() : Super() {}
	FActorRecord(const AActor* Actor) : Super(Actor) {}

	virtual bool Serialize(FArchive& Ar) override;
};


/** Represents a serialized Controller */
USTRUCT()
struct FControllerRecord : public FActorRecord {
	GENERATED_USTRUCT_BODY()

	FRotator ControlRotation;


	FControllerRecord() : Super() {}
	FControllerRecord(const AActor* Actor) : Super(Actor) {}

	virtual bool Serialize(FArchive& Ar) override;
};


/** Represents a level in the world (streaming or persistent) */
USTRUCT()
struct FLevelRecord : public FBaseRecord {
	GENERATED_USTRUCT_BODY()

	/** Record of the Level Script Actor */
	FActorRecord LevelScript;

	/** Records of the World Actors */
	TArray<FActorRecord> Actors;

	/** Records of the AI Controller Actors */
	TArray<FControllerRecord> AIControllers;


	FLevelRecord() : Super() {}

	virtual bool Serialize(FArchive& Ar) override;

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


/** Represents a serialized streaming level in the world */
USTRUCT()
struct FStreamingLevelRecord : public FLevelRecord {
	GENERATED_USTRUCT_BODY()

	FStreamingLevelRecord() : Super() {}
	FStreamingLevelRecord(const ULevelStreaming* Level) : Super()
	{
		Class = ULevelStreaming::StaticClass();
		if (Level)
			Name = Level->GetWorldAssetPackageFName();
	}

	FORCEINLINE bool operator== (const ULevelStreaming* Level) const {
		return Level && Name == Level->GetWorldAssetPackageFName();
	}
};


/** Serializes world data */
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
 * USaveData stores all information that can be accessible only while the game is loaded.
 * Works like a common SaveGame object
 * E.g: Items, Quests, Enemies, World Actors, AI, Physics
 */
UCLASS(ClassGroup = SaveExtension, hideCategories = ("Activation", "Actor Tick", "Actor", "Input", "Rendering", "Replication", "Socket", "Thumbnail"))
class SAVEEXTENSION_API USlotData : public USaveGame
{
	GENERATED_BODY()

public:

	USlotData() : Super() {}


	/** Full Name of the Map where this SlotData was saved */
	UPROPERTY(Category = SaveData, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FString Map;

	/** Game world time since game started in seconds */
	UPROPERTY(Category = SaveData, BlueprintReadOnly)
	float TimeSeconds;


	/** Records
	 * All serialized information to be saved or loaded
	 * Serialized manually for performance
	 */

	FObjectRecord GameInstance;
	FActorRecord GameMode;
	FActorRecord GameState;

	FActorRecord PlayerPawn;
	FControllerRecord PlayerController;
	FActorRecord PlayerState;
	FActorRecord PlayerHUD;

	FPersistentLevelRecord MainLevel;
	TArray<FStreamingLevelRecord> SubLevels;


	void Clean(bool bKeepLevels);
	FName GetFMap() const { return { *Map }; }

	/** Using manual serialization. It's way faster than reflection serialization */
	virtual void Serialize(FArchive& Ar) override;
};
