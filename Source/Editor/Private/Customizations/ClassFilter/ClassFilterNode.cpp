// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "ClassFilterNode.h"

#include "ClassFilter.h"

#include <Engine/Blueprint.h>
#include <PropertyHandle.h>



FSEClassFilterNode::FSEClassFilterNode(const FString& InClassName, const FString& InClassDisplayName)
{
	ClassName = InClassName;
	ClassDisplayName = InClassDisplayName;
	bPassesFilter = false;
	bIsBPNormalType = false;

	Class = nullptr;
	Blueprint = nullptr;
}

FSEClassFilterNode::FSEClassFilterNode(const FSEClassFilterNode& InCopyObject)
{
	ClassName = InCopyObject.ClassName;
	ClassDisplayName = InCopyObject.ClassDisplayName;
	bPassesFilter = InCopyObject.bPassesFilter;
	FilterState = InCopyObject.FilterState;

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
void FSEClassFilterNode::AddChild(FSEClassFilterNodePtr& Child)
{
	ChildrenList.Add(Child);
	Child->ParentNode = TSharedRef<FSEClassFilterNode>{AsShared()};
}

void FSEClassFilterNode::AddUniqueChild(FSEClassFilterNodePtr& Child)
{
	check(Child.IsValid());

	if (const UClass* NewChildClass = Child->Class.Get())
	{
		for (auto& CurrentChild : ChildrenList)
		{
			if (CurrentChild.IsValid() && CurrentChild->Class == NewChildClass)
			{
				const bool bNewChildHasMoreInfo = Child->UnloadedBlueprintData.IsValid();
				const bool bOldChildHasMoreInfo = CurrentChild->UnloadedBlueprintData.IsValid();
				if (bNewChildHasMoreInfo && !bOldChildHasMoreInfo)
				{
					// make sure, that new child has all needed children
					for (int OldChildIndex = 0; OldChildIndex < CurrentChild->ChildrenList.Num();
						 ++OldChildIndex)
					{
						Child->AddUniqueChild(CurrentChild->ChildrenList[OldChildIndex]);
					}

					// replace child
					CurrentChild = Child;
				}
				return;
			}
		}
	}

	AddChild(Child);
}

FString FSEClassFilterNode::GetClassName(EClassViewerNameTypeToDisplay NameType) const
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
			if (ClassDisplayName.IsEmpty() && !ClassDisplayName.Equals(SanitizedName) &&
				!ClassDisplayName.Equals(ClassName))
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

	ensureMsgf(false, TEXT("FSEClassFilterNode::GetClassName called with invalid name type."));
	return ClassName;
}

FText FSEClassFilterNode::GetClassTooltip(bool bShortTooltip) const
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

bool FSEClassFilterNode::IsBlueprintClass() const
{
	return !BlueprintAssetPath.IsNull();
}

void FSEClassFilterNode::SetOwnFilterState(EClassFilterState State)
{
	FilterState = State;
}

void FSEClassFilterNode::SetStateFromFilter(const FSEClassFilter& Filter)
{
	const TSoftClassPtr<> ClassAsset{ClassPath.ToString()};

	if (Filter.AllowedClasses.Contains(ClassAsset))
	{
		FilterState = EClassFilterState::Allowed;
	}
	else if (Filter.IgnoredClasses.Contains(ClassAsset))
	{
		FilterState = EClassFilterState::Denied;
	}
	else
	{
		FilterState = EClassFilterState::None;
	}
}

EClassFilterState FSEClassFilterNode::GetParentFilterState() const
{
	FSEClassFilterNodePtr Parent = ParentNode.Pin();
	while (Parent)
	{
		// return first parent found filter
		if (Parent->FilterState != EClassFilterState::None)
			return Parent->FilterState;

		Parent = Parent->ParentNode.Pin();
	}
	return EClassFilterState::None;
}
