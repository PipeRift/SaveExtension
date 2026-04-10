// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include <GameFramework/OnlineReplStructs.h>

#include "Records.generated.h"


struct FSEClassFilter;
class USaveSlotData;
class APlayerState;
class USubsystem;


USTRUCT()
struct SAVEEXTENSION_API FBaseRecord
{
	GENERATED_BODY()

	UPROPERTY()
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
struct SAVEEXTENSION_API FObjectRecord : public FBaseRecord
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UClass> Class;

	UPROPERTY()
	TArray<uint8> Data;

	UPROPERTY()
	TArray<FName> Tags;


	FObjectRecord() : Super() {}
	FObjectRecord(const UObject& Object);

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
struct SAVEEXTENSION_API FComponentRecord : public FObjectRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FTransform Transform;


	FComponentRecord() : Super() {}
	FComponentRecord(const UActorComponent& Component) : Super(Component) {}
	virtual bool Serialize(FArchive& Ar) override;
};


/** Represents a serialized Actor */
USTRUCT()
struct SAVEEXTENSION_API FActorRecord : public FObjectRecord
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHiddenInGame;
	/** Whether or not this actor was spawned in runtime */
	UPROPERTY()
	bool bIsProcedural;

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	FVector LinearVelocity = FVector::ZeroVector;

	UPROPERTY()
	FVector AngularVelocity = FVector::ZeroVector;

	UPROPERTY()
	TArray<FComponentRecord> ComponentRecords;


	FActorRecord() : bHiddenInGame(false), bIsProcedural(false) {}
	FActorRecord(const AActor& Actor);
	virtual bool Serialize(FArchive& Ar) override;
};


/** Represents a serialized Subsystem */
USTRUCT()
struct SAVEEXTENSION_API FSubsystemRecord : public FObjectRecord
{
	GENERATED_BODY()

	FSubsystemRecord() : Super() {}
	FSubsystemRecord(const USubsystem& Subsystem) : Super(Subsystem) {}
};

USTRUCT(BlueprintType)
struct SAVEEXTENSION_API FPlayerRecord
{
	GENERATED_BODY()

	FString UniqueId;

	FActorRecord PlayerState;
	FActorRecord Controller;
	FActorRecord Pawn;


	FPlayerRecord() = default;
	FPlayerRecord(const FString& UniqueId) : UniqueId(UniqueId) {}

	bool Serialize(FArchive& Ar);

	bool operator==(const FPlayerRecord& Other) const;
};

namespace SERecords
{
	extern const FName TagNoTransform;
	extern const FName TagNoPhysics;
	extern const FName TagNoTags;


	SAVEEXTENSION_API void SerializeActor(
		const AActor* Actor, FActorRecord& Record, const FSEClassFilter& ComponentFilter);
	SAVEEXTENSION_API bool DeserializeActor(
		AActor* Actor, const FActorRecord& Record, const FSEClassFilter& ComponentFilter);
	void SerializePlayer(
		const APlayerState* PlayerState, FPlayerRecord& Record, const FSEClassFilter& ComponentFilter);
	void DeserializePlayer(
		APlayerState* PlayerState, const FPlayerRecord& Record, const FSEClassFilter& ComponentFilter);

	bool IsSaveTag(const FName& Tag);
	bool StoresTransform(const AActor* Actor);
	bool StoresPhysics(const AActor* Actor);
	bool StoresTags(const AActor* Actor);
	bool IsProcedural(const AActor& Actor);
	bool StoresTags(const UActorComponent* Component);
}	 // namespace SERecords
