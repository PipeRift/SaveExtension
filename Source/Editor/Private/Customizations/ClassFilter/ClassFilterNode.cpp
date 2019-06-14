// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "ClassFilterNode.h"
#include <Engine/Blueprint.h>

#include "PropertyHandle.h"


FClassFilterNode::FClassFilterNode(const FString& InClassName, const FString& InClassDisplayName)
{
	ClassName = InClassName;
	ClassDisplayName = InClassDisplayName;
	bPassesFilter = false;
	bIsBPNormalType = false;

	Class = nullptr;
	Blueprint = nullptr;
}

FClassFilterNode::FClassFilterNode( const FClassFilterNode& InCopyObject)
{
	ClassName = InCopyObject.ClassName;
	ClassDisplayName = InCopyObject.ClassDisplayName;
	bPassesFilter = InCopyObject.bPassesFilter;

	Class = InCopyObject.Class;
	Blueprint = InCopyObject.Blueprint;

	UnloadedBlueprintData = InCopyObject.UnloadedBlueprintData;

	ClassPath = InCopyObject.ClassPath;
	ParentClassPath = InCopyObject.ParentClassPath;
	ClassName = InCopyObject.ClassName;
	BlueprintAssetPath = InCopyObject.BlueprintAssetPath;
	bIsBPNormalType = InCopyObject.bIsBPNormalType;

	// We do not want to copy the child list, do not add it. It should be the only item missing.
}

/**
 * Adds the specified child to the node.
 *
 * @param	Child							The child to be added to this node for the tree.
 */
void FClassFilterNode::AddChild(const FClassFilterNodePtr& Child)
{
	ChildrenList.Add(Child);
}

void FClassFilterNode::AddUniqueChild(const FPtr& Child)
{
	check(Child.IsValid());
	const UClass* NewChildClass = Child->Class.Get();
	if (nullptr != NewChildClass)
	{
		for (int ChildIndex = 0; ChildIndex < ChildrenList.Num(); ++ChildIndex)
		{
			FClassFilterNodePtr OldChild = ChildrenList[ChildIndex];
			if (OldChild.IsValid() && OldChild->Class == NewChildClass)
			{
				const bool bNewChildHasMoreInfo = Child->UnloadedBlueprintData.IsValid();
				const bool bOldChildHasMoreInfo = OldChild->UnloadedBlueprintData.IsValid();
				if (bNewChildHasMoreInfo && !bOldChildHasMoreInfo)
				{
					// make sure, that new child has all needed children
					for (int OldChildIndex = 0; OldChildIndex < OldChild->ChildrenList.Num(); ++OldChildIndex)
					{
						Child->AddUniqueChild(OldChild->ChildrenList[OldChildIndex]);
					}

					// replace child
					ChildrenList[ChildIndex] = Child;
				}
				return;
			}
		}
	}

	AddChild(Child);
}

FString FClassFilterNode::GetClassName(EClassViewerNameTypeToDisplay NameType) const
{
	switch (NameType)
	{
	case EClassViewerNameTypeToDisplay::ClassName:
		return ClassName;

	case EClassViewerNameTypeToDisplay::DisplayName:
		return ClassDisplayName;

	case EClassViewerNameTypeToDisplay::Dynamic:
		FString CombinedName;
		FString SanitizedName = FName::NameToDisplayString(ClassName, false);
		if (ClassDisplayName.IsEmpty() && !ClassDisplayName.Equals(SanitizedName) && !ClassDisplayName.Equals(ClassName))
		{
			TArray<FStringFormatArg> Args;
			Args.Add(ClassName);
			Args.Add(ClassDisplayName);
			CombinedName = FString::Format(TEXT("{0} ({1})"), Args);
		}
		else
		{
			CombinedName = ClassName;
		}
		return MoveTemp(CombinedName);
	}

	ensureMsgf(false, TEXT("FClassFilterNode::GetClassName called with invalid name type."));
	return ClassName;
}

FText FClassFilterNode::GetClassTooltip(bool bShortTooltip) const
{
	if (Class.IsValid())
	{
		return Class->GetToolTipText(bShortTooltip);
	}
	else if (Blueprint.IsValid() && Blueprint->GeneratedClass)
	{
		// #NOTE: Unloaded blueprint classes won't show tooltip for now
		return Blueprint->GeneratedClass->GetToolTipText(bShortTooltip);
	}

	return FText::GetEmpty();
}

bool FClassFilterNode::IsBlueprintClass() const
{
	return BlueprintAssetPath != NAME_None;
}
