
// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include "TestActor.generated.h"


USTRUCT()
struct FTestSaveStruct
{
    GENERATED_BODY()
};


UCLASS()
class ATestActor : public AActor
{
    GENERATED_BODY()

public:

    UPROPERTY(SaveGame)
    bool bMyBool = false;

    UPROPERTY(SaveGame)
    float MyFloat = 0.f;


    // INTEGERS

    UPROPERTY(SaveGame)
    uint8 MyU8 = 0.f;

    UPROPERTY(SaveGame)
    uint16 MyU16 = 0;

    UPROPERTY(SaveGame)
    uint32 MyU32 = 0;

    UPROPERTY(SaveGame)
    uint64 MyU64 = 0;

    UPROPERTY(SaveGame)
    int8 MyI8 = 0.f;

    UPROPERTY(SaveGame)
    int16 MyI16 = 0;

    UPROPERTY(SaveGame)
    int32 MyI32 = 0;

    UPROPERTY(SaveGame)
    int64 MyI64 = 0;


    UPROPERTY(SaveGame)
    FTestSaveStruct MyStruct;
};
