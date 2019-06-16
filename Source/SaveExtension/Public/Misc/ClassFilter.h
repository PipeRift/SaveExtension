// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ClassFilter.generated.h"


USTRUCT(BlueprintType)
struct SAVEEXTENSION_API FClassFilter
{
	GENERATED_BODY()

	// Used from editor side to limit displayed classes
	UPROPERTY()
	const UClass* BaseClass;

	/** This classes are allowed (and their children) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization")
	TSet<TSoftClassPtr<UObject>> AllowedClasses;

	/** This classes are ignored (and their children) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Serialization")
	TSet<TSoftClassPtr<UObject>> IgnoredClasses;

	UPROPERTY(Transient)
	TSet<const UClass*> BakedAllowedClasses;


	FClassFilter() : FClassFilter(UObject::StaticClass()) {}
	FClassFilter(const UClass* BaseClass);

	/** Bakes a set of allowed classes based on the current settings */
	void BakeAllowedClasses();

	bool IsClassAllowed(const UClass* Class) const {
		// Check is a single O(1) pointer hash comparison
		return BakedAllowedClasses.Contains(Class);
	}
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
};
