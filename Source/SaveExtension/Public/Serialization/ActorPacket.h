// Copyright 2015-2019 Piperift. All Rights Reserved
#pragma once

#include <CoreMinimal.h>
#include <Engine/Level.h>
#include <GameFramework/Actor.h>

#include "Records.h"
#include "ObjectPacket.h"
#include "ClassFilter.h"
#include "ActorPacket.generated.h"


UENUM(BlueprintType)
enum class EPacketLevelMode : uint8 {
	RootLevelOnly,  // Affects only root levels
	SubLevelsOnly,  // Affects only sub-levels
	AllLevels,      // Affects all levels
	SpecifiedLevels // Affects levels specified on "LevelList"
};


USTRUCT(BlueprintType)
struct FActorPacketSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	EPacketLevelMode Levels = EPacketLevelMode::AllLevels;

	/** List of levels to affect if Packet is "SpecifiedLevels" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	TArray<TSoftObjectPtr<UWorld>> LevelList;

	/** List of levels to affect if Packet is "SpecifiedLevels" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	bool bDontCheckRootLevel = false;


	bool IsLevelAllowed(FName Name) const;
};


USTRUCT()
struct FActorPacketRecord : public FObjectPacketRecord
{
	GENERATED_BODY()

	/** Records of the World Actors */
	TArray<FActorRecord> Actors;

	// CustomSerializer*


	FActorPacketRecord() : Super(FClassFilter{ AActor::StaticClass() }) {}
	FActorPacketRecord(const FActorClassFilter& InFilter)
		: Super(InFilter.ClassFilter)
	{}

	bool operator==(const FActorPacketRecord& Other) const;
};
