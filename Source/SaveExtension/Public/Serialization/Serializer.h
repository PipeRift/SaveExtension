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

	inline void DoSerialize(FArchive& Ar, UObject* Object);
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
UCLASS(Blueprintable)
class UInstanceSerializer : public USerializer
{
	GENERATED_BODY()

	FArchive* CurrentArchive = nullptr;


public:

	UInstanceSerializer() : Super(ESerializerType::Instance) {}

	void DoSerialize(FArchive& Ar, UObject* Object) {
		CurrentArchive = &Ar;
		EventSerialize(Object);
		CurrentArchive = nullptr;
	}

	UFUNCTION(BlueprintPure, Category = Serializer)
	FORCEINLINE bool IsSaving()  const { return CurrentArchive->IsSaving(); }

	UFUNCTION(BlueprintPure, Category = Serializer)
	FORCEINLINE bool IsLoading() const { return CurrentArchive->IsLoading(); }

protected:

	virtual void SerializeInstance(FArchive& Ar, UObject* Instance) {}

	UFUNCTION(BlueprintNativeEvent, Category = Serializer, meta = (ForceAsFunction, DisplayName = "Serialize"))
	void EventSerialize(UObject* Instance);
	void EventSerialize_Implementation(UObject* Instance)
	{
		SerializeInstance(*CurrentArchive, Instance);
	}


	UFUNCTION(BlueprintCallable, Category = "Serializer|Types")
	void SerializeBool(UPARAM(Ref) bool& Value) { *CurrentArchive << Value; }

	UFUNCTION(BlueprintCallable, Category = "Serializer|Types")
	void SerializeByte(UPARAM(Ref) uint8& Value) { *CurrentArchive << Value; }

	UFUNCTION(BlueprintCallable, Category = "Serializer|Types")
	void SerializeFloat(UPARAM(Ref) float& Value) { *CurrentArchive << Value; }

	UFUNCTION(BlueprintCallable, Category = "Serializer|Types")
	void SerializeInt(UPARAM(Ref) int32& Value) { *CurrentArchive << Value; }

	UFUNCTION(BlueprintCallable, Category = "Serializer|Types")
	void SerializeInt64(UPARAM(Ref) int64& Value) { *CurrentArchive << Value; }

	UFUNCTION(BlueprintCallable, Category = "Serializer|Types")
	void SerializeName(UPARAM(Ref) FName& Value) { *CurrentArchive << Value; }

	UFUNCTION(BlueprintCallable, Category = "Serializer|Types")
	void SerializeString(UPARAM(Ref) FString& Value) { *CurrentArchive << Value; }

	UFUNCTION(BlueprintCallable, Category = "Serializer|Types")
	void SerializeText(UPARAM(Ref) FText& Value) { *CurrentArchive << Value; }

	UFUNCTION(BlueprintCallable, Category = "Serializer|Types")
	void SerializeVector(UPARAM(Ref) FVector& Value) { *CurrentArchive << Value; }

	UFUNCTION(BlueprintCallable, Category = "Serializer|Types")
	void SerializeVector2D(UPARAM(Ref) FVector2D& Value) { *CurrentArchive << Value; }

	UFUNCTION(BlueprintCallable, Category = "Serializer|Types")
	void SerializeIntPoint(UPARAM(Ref) FIntPoint& Value) { *CurrentArchive << Value; }

	UFUNCTION(BlueprintCallable, Category = "Serializer|Types")
	void SerializeIntVector(UPARAM(Ref) FIntVector& Value) { *CurrentArchive << Value; }

	UFUNCTION(BlueprintCallable, Category = "Serializer|Types")
	void SerializeQuat(UPARAM(Ref) FQuat& Value) { *CurrentArchive << Value; }

	UFUNCTION(BlueprintCallable, Category = "Serializer|Types")
	void SerializeRotator(UPARAM(Ref) FRotator& Value) { *CurrentArchive << Value; }

	UFUNCTION(BlueprintCallable, Category = "Serializer|Types")
	void SerializeObject(UPARAM(Ref) UObject* Value) {}
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
