// Copyright 2015-2024 Piperift. All Rights Reserved.


#pragma once

#include <CoreMinimal.h>
#include <Engine/LevelScriptActor.h>
#include <Engine/LevelStreaming.h>

#include "Records.generated.h"


class USaveSlotData;


USTRUCT()
struct FBaseRecord
{
	GENERATED_BODY()

	FName Name;


	FBaseRecord() : Name() {}

	virtual bool Serialize(FArchive& Ar);
	friend FArchive& operator<<(FArchive& Ar, FBaseRecord& Record)
	{
		Record.Serialize(Ar);
		return Ar;
	}
	virtual ~FBaseRecord() {}
};

template <>
struct TStructOpsTypeTraits<FBaseRecord> : public TStructOpsTypeTraitsBase2<FBaseRecord>
{
	enum
	{
		WithSerializer = true
	};
};

inline bool operator==(const FBaseRecord& A, const FBaseRecord& B)
{
	return A.Name == B.Name;
}


/** Represents a serialized Object */
USTRUCT()
struct FObjectRecord : public FBaseRecord
{
	GENERATED_BODY()

	UPROPERTY()
	UClass* Class;

	TArray<uint8> Data;
	TArray<FName> Tags;


	FObjectRecord() : Super(), Class(nullptr) {}
	FObjectRecord(const UObject* Object);

	virtual bool Serialize(FArchive& Ar) override;

	bool IsValid() const
	{
		return !Name.IsNone() && Class;
	}

	bool operator==(const UObject* Other) const
	{
		return Other && Name == Other->GetFName() && Class == Other->GetClass();
	}
};


/** Represents a serialized Component */
USTRUCT()
struct FComponentRecord : public FObjectRecord
{
	GENERATED_BODY()

	FTransform Transform;


	virtual bool Serialize(FArchive& Ar) override;
};


/** Represents a serialized Actor */
USTRUCT()
struct FActorRecord : public FObjectRecord
{
	GENERATED_BODY()

	bool bHiddenInGame;
	/** Whether or not this actor was spawned in runtime */
	bool bIsProcedural;
	FTransform Transform;
	FVector LinearVelocity = FVector::ZeroVector;
	FVector AngularVelocity = FVector::ZeroVector;
	TArray<FComponentRecord> ComponentRecords;


	FActorRecord() : Super() {}
	FActorRecord(const AActor* Actor) : Super(Actor) {}

	virtual bool Serialize(FArchive& Ar) override;
};


/** Represents a serialized Subsystem */
USTRUCT()
struct FSubsystemRecord : public FObjectRecord
{
	GENERATED_BODY()

	FSubsystemRecord() : Super() {}
	FSubsystemRecord(const USubsystem* Subsystem) : Super(Subsystem) {}
};