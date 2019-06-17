// Copyright 2015-2019 Piperift. All Rights Reserved.


#pragma once

#include <CoreMinimal.h>
#include <Engine/LevelStreaming.h>
#include <Engine/LevelScriptActor.h>

#include "Records.h"
#include "ActorPacket.h"
#include "LevelRecords.generated.h"


/** Represents a level in the world (streaming or persistent) */
USTRUCT()
struct FLevelRecord : public FBaseRecord
{
	GENERATED_BODY()

	/** Record of the Level Script Actor */
	FActorRecord LevelScript;

	/** Records of the World Actors */
	TArray<FActorRecord> Actors;

	/** Records of the AI Controller Actors */
	TArray<FControllerRecord> AIControllers;


	TArray<FActorPacketRecord> ActorPackets;

	// Combined filter from all ComponentPackets
	FClassFilter ComponentFilter;


	FLevelRecord() : Super() {}

	virtual bool Serialize(FArchive& Ar) override;

	bool IsValid() const {
		return !Name.IsNone();
	}

	void Clean();

	FActorPacketRecord& FindOrCreateActorPacket(const FActorPacketRecord& NewPacket);
};


/** Represents a persistent level in the world */
USTRUCT()
struct FPersistentLevelRecord : public FLevelRecord
{
	GENERATED_BODY()

	static FName PersistentName;

	FPersistentLevelRecord() : Super() { Name = PersistentName; }
};


/** Represents a serialized streaming level in the world */
USTRUCT()
struct FStreamingLevelRecord : public FLevelRecord
{
	GENERATED_BODY()

	FStreamingLevelRecord() : Super() {}
	FStreamingLevelRecord(const ULevelStreaming* Level) : Super()
	{
		if (Level)
		{
			Name = Level->GetWorldAssetPackageFName();
		}
	}

	FORCEINLINE bool operator== (const ULevelStreaming* Level) const
	{
		return Level && Name == Level->GetWorldAssetPackageFName();
	}
};
