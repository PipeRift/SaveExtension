// Copyright 2015-2020 Piperift. All Rights Reserved.
#pragma once

#include <CoreMinimal.h>
#include <ClassViewerModule.h>
#include <UObject/WeakObjectPtr.h>
#include <Templates/SharedPointer.h>

class IPropertyHandle;
class IUnloadedBlueprintData;
class UBlueprint;


enum class EClassFilterState : uint8
{
	Allowed,
	Denied,
	None
};


class FClassFilterNode : public TSharedFromThis<FClassFilterNode>
{
public:

	using FPtr = TSharedPtr<FClassFilterNode>;

	/**
	 * Creates a node for the widget's tree.
	 *
	 * @param	InClassName						The name of the class this node represents.
	 * @param	InClassDisplayName				The display name of the class this node represents
	 * @param	bInIsPlaceable					true if the class is a placeable class.
	 */
	FClassFilterNode( const FString& InClassName, const FString& InClassDisplayName );

	FClassFilterNode( const FClassFilterNode& InCopyObject);

	/**
	 * Adds the specified child to the node.
	 *
	 * @param	Child							The child to be added to this node for the tree.
	 */
	void AddChild(FPtr& Child);
	void AddUniqueChild(FPtr& Child);

	/**
	 * Retrieves the class name this node is associated with. This is not the literal UClass name as it is missing the _C for blueprints
	 * @param	bUseDisplayName	Whether to use the display name or class name
	 */
	const FString& GetClassName(bool bUseDisplayName = false) const
	{
		return bUseDisplayName ? ClassDisplayName : ClassName;
	}

	/**
	 * Retrieves the class name this node is associated with. This is not the literal UClass name as it is missing the _C for blueprints
	 * @param	NameType	Whether to use the display name or class name
	 */
	FString GetClassName(EClassViewerNameTypeToDisplay NameType) const;

	FText GetClassTooltip(bool bShortTooltip = false) const;

	/** Retrieves the children list. */
	TArray<FPtr>& GetChildrenList()
	{
		return ChildrenList;
	}

	/** Checks if this is a blueprint */
	bool IsBlueprintClass() const;


	/** Filter states */
	void SetOwnFilterState(EClassFilterState State);
	void SetStateFromFilter(const struct FClassFilter& Filter);
	EClassFilterState GetOwnFilterState() const { return FilterState; }
	EClassFilterState GetParentFilterState() const;

private:

	/** The non-translated internal name for this class. This is not necessarily the UClass's name, as that may have _C for blueprints */
	FString ClassName;

	/** The translated display name for this class */
	FString ClassDisplayName;

	/** List of children. */
	TArray<FPtr> ChildrenList;

	EClassFilterState FilterState = EClassFilterState::None;

public:

	TWeakPtr<FClassFilterNode> ParentNode;

	/** The class this node is associated with. */
	TWeakObjectPtr<UClass> Class;

	/** The blueprint this node is associated with. */
	TWeakObjectPtr<UBlueprint> Blueprint;

	/** Full object path to the class including _C, set for both blueprint and native */
	FName ClassPath;

	/** Full object path to the parent class, may be blueprint or native */
	FName ParentClassPath;

	/** Full path to the Blueprint that this class is loaded from, none for native classes*/
	FName BlueprintAssetPath;

	/** true if the class passed the filter. */
	bool bPassesFilter;

	/** true if the class is a "normal type", this is used to identify unloaded blueprints as blueprint bases. */
	bool bIsBPNormalType;

	/** Data for unloaded blueprints, only valid if the class is unloaded. */
	TSharedPtr<class IUnloadedBlueprintData> UnloadedBlueprintData;
};

using FClassFilterNodePtr = FClassFilterNode::FPtr;
