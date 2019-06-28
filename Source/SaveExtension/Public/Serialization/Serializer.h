// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Serializer.generated.h"


enum class ESerializerType : uint8 {
	None,
	CDO,
	Instance,
	Blueprints
};


/**
 * Used to optionally extend on default engine serialization from c++ or blueprints
 */
UCLASS()
class USerializer : public UObject
{
	GENERATED_BODY()

	ESerializerType Type;


protected:
	USerializer() : Super(), Type(ESerializerType::None) {}

	/** Used to avoid virtual function costs on serialize calls. See DoSerialize() */
	USerializer(ESerializerType Type) : Super(), Type(Type) {}

public:

	void DoSerialize(FArchive& Ar, UObject* Object);
};

/**
 * Used to optionally extend on default engine serialization from c++ or blueprints
 * A CDO Serializer selects what to save once from its CDO and uses the same rule for all serialized objects
*/
UCLASS()
class UCDOSerializer : public USerializer
{
	GENERATED_BODY()

public:

	UCDOSerializer() : Super(ESerializerType::CDO) {}

	void DoSerialize(FArchive& Ar, UObject* Object) {}

	virtual void SerializeCDO(UObject* CDO) {}
};

/**
* Used to optionally extend on default engine serialization from c++ or blueprints
* A CDO Serializer selects what to save once from its CDO and uses the same rule for all serialized objects
*/
UCLASS()
class UInstanceSerializer : public USerializer
{
	GENERATED_BODY()

	FArchive* CurrentArchive;


public:

	UInstanceSerializer() : Super(ESerializerType::Instance) {}

	void DoSerialize(FArchive& Ar, UObject* Object) {}

	virtual void SerializeInstance(UObject* Instance) {}

	template<typename T>
	FORCEINLINE UInstanceSerializer& operator<<(T value) {
		CurrentArchive << value;
	}

	FORCEINLINE FArchive* GetArchive() const { return CurrentArchive; }
	FORCEINLINE bool IsSaving()  const { return CurrentArchive->IsSaving(); }
	FORCEINLINE bool IsLoading() const { return CurrentArchive->IsLoading(); }
};

/**
* Used to optionally extend on default engine serialization from c++ or blueprints
*/
UCLASS()
class UBlueprintSerializer : public USerializer
{
	GENERATED_BODY()

public:

	UBlueprintSerializer() : Super(ESerializerType::Blueprints) {}

	void DoSerialize(FArchive& Ar, UObject* Object) {}

	void EventSerialize(UObject* Object) {}
};
