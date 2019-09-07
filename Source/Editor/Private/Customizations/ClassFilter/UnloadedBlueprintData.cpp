// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "UnloadedBlueprintData.h"
#include "Engine/BlueprintGeneratedClass.h"


FUnloadedBlueprintData::FUnloadedBlueprintData(TWeakPtr<FClassFilterNode> ClassFilterNode)
	: ClassFilterNode(ClassFilterNode)
{
}

bool FUnloadedBlueprintData::HasAnyClassFlags( uint32 InFlagsToCheck ) const
{
	return (ClassFlags & InFlagsToCheck) != 0;
}

bool FUnloadedBlueprintData::HasAllClassFlags( uint32 InFlagsToCheck ) const
{
	return ((ClassFlags & InFlagsToCheck) == InFlagsToCheck);
}

void FUnloadedBlueprintData::SetClassFlags(uint32 InFlags)
{
	ClassFlags = InFlags;
}

bool FUnloadedBlueprintData::IsChildOf(const UClass* InClass) const
{
	FClassFilterNodePtr CurrentNode = ClassFilterNode.Pin()->ParentNode.Pin();

	// Keep going through parents till you find an invalid.
	while (CurrentNode.IsValid())
	{
		if (CurrentNode->Class.Get() == InClass)
		{
			return true;
		}
		CurrentNode = CurrentNode->ParentNode.Pin();
	}

	return false;
}

bool FUnloadedBlueprintData::ImplementsInterface(const UClass* InInterface) const
{
	// Does this blueprint implement the interface directly?
	for (const FString& DirectlyImplementedInterface : ImplementedInterfaces)
	{
		if (DirectlyImplementedInterface == InInterface->GetName())
		{
			return true;
		}
	}

	// If not, does a parent class implement the interface?
	FClassFilterNodePtr CurrentNode = ClassFilterNode.Pin()->ParentNode.Pin();
	while (CurrentNode.IsValid())
	{
		if (CurrentNode->Class.IsValid() && CurrentNode->Class->ImplementsInterface(InInterface))
		{
			return true;
		}
		else if (CurrentNode->UnloadedBlueprintData.IsValid() && CurrentNode->UnloadedBlueprintData->ImplementsInterface(InInterface))
		{
			return true;
		}
		CurrentNode = CurrentNode->ParentNode.Pin();
	}

	return false;
}

bool FUnloadedBlueprintData::IsA(const UClass* InClass) const
{
	// Unloaded blueprints will always return true for IsA(UBlueprintGeneratedClass::StaticClass). With that in mind, even though we do not have the exact class, we can use that knowledge as a basis for a check.
	return ((UObject*)UBlueprintGeneratedClass::StaticClass())->IsA(InClass);
}

const UClass* FUnloadedBlueprintData::GetClassWithin() const
{
	FClassFilterNodePtr CurrentNode = ClassFilterNode.Pin()->ParentNode.Pin();

	while (CurrentNode.IsValid())
	{
		// The class field will be invalid for unloaded classes.
		// However, it should be valid once we've hit a loaded class or a Native class.
		// Assuming BP cannot change ClassWithin data, this should be safe.
		if (CurrentNode->Class.IsValid())
		{
			return CurrentNode->Class->ClassWithin;
		}

		CurrentNode = CurrentNode->ParentNode.Pin();
	}

	return nullptr;
}

const UClass* FUnloadedBlueprintData::GetNativeParent() const
{
	FClassFilterNodePtr CurrentNode = ClassFilterNode.Pin()->ParentNode.Pin();

	while (CurrentNode.IsValid())
	{
		// The class field will be invalid for unloaded classes.
		// However, it should be valid once we've hit a loaded class or a Native class.
		// Assuming BP cannot change ClassWithin data, this should be safe.
		if (CurrentNode->Class.IsValid() && CurrentNode->Class->HasAnyClassFlags(CLASS_Native))
		{
			return CurrentNode->Class.Get();
		}

		CurrentNode = CurrentNode->ParentNode.Pin();
	}

	return nullptr;
}


const TWeakPtr<FClassFilterNode>& FUnloadedBlueprintData::GetClassFilterNode() const
{
	return ClassFilterNode;
}

void FUnloadedBlueprintData::AddImplementedInterface(const FString& InterfaceName)
{
	ImplementedInterfaces.Add(InterfaceName);
}
