// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Editor/ClassViewer/Private/ClassViewerNode.h"
#include "ClassFilterNode.h"
#include <ClassViewerFilter.h>

class FUnloadedBlueprintData : public IUnloadedBlueprintData
{
public:
	FUnloadedBlueprintData(TWeakPtr<FClassFilterNode> ClassFilterNode);

	virtual ~FUnloadedBlueprintData() {}

	virtual bool HasAnyClassFlags( uint32 InFlagsToCheck ) const override;

	virtual bool HasAllClassFlags( uint32 InFlagsToCheck ) const override;

	virtual void SetClassFlags(uint32 InFlags) override;

	virtual bool ImplementsInterface(const UClass* InInterface) const override;

	virtual bool IsChildOf(const UClass* InClass) const override;

	virtual bool IsA(const UClass* InClass) const override;

	virtual const UClass* GetClassWithin() const override;

	virtual const UClass* GetNativeParent() const override;

	/** Retrieves the Class Viewer node this data is associated with. */
	const TWeakPtr<FClassFilterNode>& GetClassFilterNode() const;

	/** Adds the name of an interface that this blueprint implements directly. */
	void AddImplementedInterface(const FString& InterfaceName);

private:
	/** Flags for the class. */
	uint32 ClassFlags = CLASS_None;

	/** The implemented interfaces for this class. */
	TArray<FString> ImplementedInterfaces;

	/** The node this class is contained in, used to gather hierarchical data as needed. */
	TWeakPtr<FClassFilterNode> ClassFilterNode;
};
