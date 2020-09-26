// Copyright 2015-2020 Piperift. All Rights Reserved.


#pragma once

#include <CoreMinimal.h>
#include <Engine/LevelStreaming.h>
#include <Engine/LevelScriptActor.h>

#include "Records.h"
#include "SaveFilter.h"
#include "LevelRecords.generated.h"


/** Represents a level in the world (streaming or persistent) */
USTRUCT()
struct FLevelRecord : public FBaseRecord
{
	GENERATED_BODY()


	FSaveFilter SaveFilter;

	/** Record of the Level Script Actor */
	FActorRecord LevelScript;

	/** Records of the World Actors */
	TArray<FActorRecord> Actors;


	FLevelRecord() : Super() {}

	virtual bool Serialize(FArchive& Ar) override;

	bool IsValid() const { return !Name.IsNone(); }

	void Clean();
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
