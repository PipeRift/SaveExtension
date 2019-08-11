// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include <GameFramework/Actor.h>
#include <Components/ActorComponent.h>
#include "ClassFilter.generated.h"


USTRUCT(BlueprintType)
struct SAVEEXTENSION_API FClassFilter
{
	GENERATED_BODY()

private:

	// Used from editor side to limit displayed classes
	UPROPERTY()
	UClass* BaseClass;

public:

	/** This classes are allowed (and their children) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization")
	TSet<TSoftClassPtr<UObject>> AllowedClasses;

	/** This classes are ignored (and their children) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization")
	TSet<TSoftClassPtr<UObject>> IgnoredClasses;

protected:

	UPROPERTY(Transient)
	mutable TSet<const UClass*> BakedAllowedClasses;


public:

	FClassFilter() : FClassFilter(UObject::StaticClass()) {}
	FClassFilter(UClass* const BaseClass);

	// Merges another filter into this one. Other has priority.
	void Merge(const FClassFilter& Other);

	/** Bakes a set of allowed classes based on the current settings */
	void BakeAllowedClasses() const;

	FORCEINLINE bool IsClassAllowed(UClass* const Class) const
	{
		// Check is a single O(1) pointer hash comparison
		return BakedAllowedClasses.Contains(Class);
	}

	FORCEINLINE UClass* GetBaseClass() const { return BaseClass; }

	FString ToString();
	void FromString(FString String);

	bool operator==(const FClassFilter& Other) const;
};


USTRUCT(BlueprintType)
struct FActorClassFilter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Filter)
	FClassFilter ClassFilter;


	FActorClassFilter()
		: ClassFilter(AActor::StaticClass())
	{}
	FActorClassFilter(TSubclassOf<AActor> actorClass) : ClassFilter(actorClass) {}

	/** Bakes a set of allowed classes based on the current settings */
	void BakeAllowedClasses() const { ClassFilter.BakeAllowedClasses(); }

	FORCEINLINE bool IsClassAllowed(UClass* const Class) const
	{
		return ClassFilter.IsClassAllowed(Class);
	}
};


USTRUCT(BlueprintType)
struct FComponentClassFilter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Filter)
	FClassFilter ClassFilter;


	FComponentClassFilter()
		: ClassFilter(UActorComponent::StaticClass())
	{}
	FComponentClassFilter(TSubclassOf<UActorComponent> compClass) : ClassFilter(compClass) {}

	/** Bakes a set of allowed classes based on the current settings */
	void BakeAllowedClasses() const { ClassFilter.BakeAllowedClasses(); }

	FORCEINLINE bool IsClassAllowed(UClass* const Class) const
	{
		return ClassFilter.IsClassAllowed(Class);
	}
};
