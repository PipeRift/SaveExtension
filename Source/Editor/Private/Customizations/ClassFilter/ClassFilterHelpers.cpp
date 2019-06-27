// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "ClassFilterHelpers.h"
#include <HotReloadInterface.h>
#include <Editor.h>
#include <CoreRedirects.h>
#include <Animation/AnimBlueprint.h>

#include "UnloadedBlueprintData.h"


namespace ClassFilter
{
	static void OnModulesChanged(FName ModuleThatChanged, EModuleChangeReason ReasonForChange)
	{
		ClassFilter::Helpers::RequestPopulateClassHierarchy();
	}

	FClassHierarchy::FClassHierarchy()
	{
		// Register with the Asset Registry to be informed when it is done loading up files.
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		OnFilesLoadedRequestPopulateClassHierarchyDelegateHandle = AssetRegistryModule.Get().OnFilesLoaded().AddStatic(ClassFilter::Helpers::RequestPopulateClassHierarchy);
		AssetRegistryModule.Get().OnAssetAdded().AddRaw(this, &FClassHierarchy::AddAsset);
		AssetRegistryModule.Get().OnAssetRemoved().AddRaw(this, &FClassHierarchy::RemoveAsset);

		// Register to have Populate called when doing a Hot Reload.
		IHotReloadInterface& HotReloadSupport = FModuleManager::LoadModuleChecked<IHotReloadInterface>("HotReload");
		HotReloadSupport.OnHotReload().AddRaw(this, &FClassHierarchy::OnHotReload);

		// Register to have Populate called when a Blueprint is compiled.
		OnBlueprintCompiledRequestPopulateClassHierarchyDelegateHandle = GEditor->OnBlueprintCompiled().AddStatic(ClassFilter::Helpers::RequestPopulateClassHierarchy);
		OnClassPackageLoadedOrUnloadedRequestPopulateClassHierarchyDelegateHandle = GEditor->OnClassPackageLoadedOrUnloaded().AddStatic(ClassFilter::Helpers::RequestPopulateClassHierarchy);

		FModuleManager::Get().OnModulesChanged().AddStatic(&OnModulesChanged);
	}

	FClassHierarchy::~FClassHierarchy()
	{
		// Unregister with the Asset Registry to be informed when it is done loading up files.
		if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
		{
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			AssetRegistryModule.Get().OnFilesLoaded().Remove(OnFilesLoadedRequestPopulateClassHierarchyDelegateHandle);
			AssetRegistryModule.Get().OnAssetAdded().RemoveAll(this);
			AssetRegistryModule.Get().OnAssetRemoved().RemoveAll(this);

			// Unregister to have Populate called when doing a Hot Reload.
			if (FModuleManager::Get().IsModuleLoaded("HotReload"))
			{
				IHotReloadInterface& HotReloadSupport = FModuleManager::GetModuleChecked<IHotReloadInterface>("HotReload");
				HotReloadSupport.OnHotReload().RemoveAll(this);
			}

			if (GEditor)
			{
				// Unregister to have Populate called when a Blueprint is compiled.
				GEditor->OnBlueprintCompiled().Remove(OnBlueprintCompiledRequestPopulateClassHierarchyDelegateHandle);
				GEditor->OnClassPackageLoadedOrUnloaded().Remove(OnClassPackageLoadedOrUnloadedRequestPopulateClassHierarchyDelegateHandle);
			}
		}

		FModuleManager::Get().OnModulesChanged().RemoveAll(this);
	}

	static FClassFilterNodePtr CreateNodeForClass(UClass* Class, const TMultiMap<FName, FAssetData>& BlueprintPackageToAssetDataMap)
	{
		// Create the new node so it can be passed to AddChildren, fill it in with if it is placeable, abstract, and/or a brush.
		TSharedPtr<FClassFilterNode> NewNode = MakeShared<FClassFilterNode>(Class->GetName(), Class->GetDisplayNameText().ToString());
		NewNode->Blueprint = ClassFilter::Helpers::GetBlueprint(Class);
		NewNode->Class = Class;
		NewNode->ClassPath = FName(*Class->GetPathName());
		if (Class->GetSuperClass())
		{
			NewNode->ParentClassPath = FName(*Class->GetSuperClass()->GetPathName());
		}

		return NewNode;
	}

	void FClassHierarchy::OnHotReload(bool bWasTriggeredAutomatically)
	{
		ClassFilter::Helpers::RequestPopulateClassHierarchy();
	}

	void FClassHierarchy::AddChildren_NoFilter(FClassFilterNodePtr& InOutRootNode, const TMultiMap<FName, FAssetData>& BlueprintPackageToAssetDataMap)
	{
		UClass* RootClass = UObject::StaticClass();

		ObjectClassRoot = MakeShared<FClassFilterNode>(RootClass->GetName(), RootClass->GetDisplayNameText().ToString());
		ObjectClassRoot->Class = RootClass;

		TMap<UClass*, FClassFilterNodePtr> Nodes;

		Nodes.Add(RootClass, ObjectClassRoot);

		TSet<UClass*> Visited;
		Visited.Add(RootClass);

		// Go through all of the classes children and see if they should be added to the list.
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			UClass* CurrentClass = *ClassIt;

			// Ignore deprecated and temporary trash classes.
			if (CurrentClass->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists) ||
				FKismetEditorUtilities::IsClassABlueprintSkeleton(CurrentClass))
			{
				continue;
			}

			FClassFilterNodePtr& Entry = Nodes.FindOrAdd(CurrentClass);
			if (Visited.Contains(CurrentClass))
			{
				continue;
			}
			else
			{
				while (CurrentClass->GetSuperClass() != nullptr)
				{
					FClassFilterNodePtr& ParentEntry = Nodes.FindOrAdd(CurrentClass->GetSuperClass());
					if (!ParentEntry.IsValid())
					{
						ParentEntry = CreateNodeForClass(CurrentClass->GetSuperClass(), BlueprintPackageToAssetDataMap);
					}

					FClassFilterNodePtr& MyEntry = Nodes.FindOrAdd(CurrentClass);
					if (!MyEntry.IsValid())
					{
						MyEntry = CreateNodeForClass(CurrentClass, BlueprintPackageToAssetDataMap);
					}

					if (!Visited.Contains(CurrentClass))
					{
						ParentEntry->AddChild(MyEntry);
						Visited.Add(CurrentClass);
					}

					CurrentClass = CurrentClass->GetSuperClass();
				}
			}
		}
	}

	FClassFilterNodePtr FClassHierarchy::FindParent(const FClassFilterNodePtr& InRootNode, FName InParentClassname, const UClass* InParentClass)
	{
		// Check if the current node is the parent class name that is being searched for.
		if (InRootNode->ClassPath == InParentClassname)
		{
			// Return the node if it is the correct parent, this ends the recursion.
			return InRootNode;
		}
		else
		{
			// If a class does not have a generated class name, we look up the parent class and compare.
			const UClass* ParentClass = InParentClass;

			if (const UClass * RootClass = InRootNode->Class.Get())
			{
				if (ParentClass == RootClass)
				{
					return InRootNode;
				}
			}

		}

		// Search the children recursively, one of them might have the parent.
		FClassFilterNodePtr ReturnNode;
		for (const auto& Child : InRootNode->GetChildrenList())
		{
			// Check the child, then check the return to see if it is valid. If it is valid, end the recursion.
			ReturnNode = FindParent(Child, InParentClassname, InParentClass);
			if (ReturnNode.IsValid())
			{
				return ReturnNode;
			}
		}
		return {};
	}

	FClassFilterNodePtr FClassHierarchy::FindNodeByClassName(const FClassFilterNodePtr& InRootNode, const FString& InClassName)
	{
		FString NodeClassName = InRootNode->Class.IsValid() ? InRootNode->Class->GetPathName() : FString();
		if (NodeClassName == InClassName)
		{
			return InRootNode;
		}

		// Search the children recursively, one of them might have the parent.
		FClassFilterNodePtr ReturnNode;
		for (const auto& Child : InRootNode->GetChildrenList())
		{
			// Check the child, then check the return to see if it is valid. If it is valid, end the recursion.
			ReturnNode = FindNodeByClassName(Child, InClassName);
			if (ReturnNode.IsValid())
			{
				return ReturnNode;
			}
		}
		return {};
	}

	FClassFilterNodePtr FClassHierarchy::FindNodeByGeneratedClassPath(const FClassFilterNodePtr& InRootNode, FName InGeneratedClassPath)
	{
		if (InRootNode->ClassPath == InGeneratedClassPath)
		{
			return InRootNode;
		}

		// Search the children recursively, one of them might have the parent.
		FClassFilterNodePtr ReturnNode;
		for (const auto& Child : InRootNode->GetChildrenList())
		{
			// Check the child, then check the return to see if it is valid. If it is valid, end the recursion.
			ReturnNode = FindNodeByGeneratedClassPath(Child, InGeneratedClassPath);
			if (ReturnNode.IsValid())
			{
				return ReturnNode;
			}
		}
		return {};
	}

	void FClassHierarchy::UpdateClassInNode(FName InGeneratedClassPath, UClass* InNewClass, UBlueprint* InNewBluePrint)
	{
		FClassFilterNodePtr Node = FindNodeByGeneratedClassPath(ObjectClassRoot, InGeneratedClassPath);

		if (Node.IsValid())
		{
			Node->Class = InNewClass;
			Node->Blueprint = InNewBluePrint;
		}
	}

	bool FClassHierarchy::FindAndRemoveNodeByClassPath(const FClassFilterNodePtr& InRootNode, FName InClassPath)
	{
		bool bReturnValue = false;

		// Search the children recursively, one of them might have the parent.
		FClassFilterNodePtr ReturnNode;
		for (int32 i = 0; i < InRootNode->GetChildrenList().Num(); ++i)
		{
			const auto& Child = InRootNode->GetChildrenList()[i];
			if (Child->ClassPath == InClassPath)
			{
				InRootNode->GetChildrenList().RemoveAt(i);
				return true;
			}

			// Check the child, then check the return to see if it is valid. If it is valid, end the recursion.
			bReturnValue |= FindAndRemoveNodeByClassPath(Child, InClassPath);
			if (bReturnValue)
			{
				break;
			}
		}
		return bReturnValue;
	}

	void FClassHierarchy::RemoveAsset(const FAssetData& InRemovedAssetData)
	{
		FString ClassObjectPath;
		if (InRemovedAssetData.GetTagValue(FBlueprintTags::GeneratedClassPath, ClassObjectPath))
		{
			ClassObjectPath = FPackageName::ExportTextPathToObjectPath(ClassObjectPath);
		}

		if (FindAndRemoveNodeByClassPath(ObjectClassRoot, FName(*ClassObjectPath)))
		{
			// All viewers must refresh.
			ClassFilter::Helpers::RefreshAll();
		}
	}

	void FClassHierarchy::AddAsset(const FAssetData& InAddedAssetData)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		if (!AssetRegistryModule.Get().IsLoadingAssets())
		{
			TArray<FName> AncestorClassNames;
			AssetRegistryModule.Get().GetAncestorClassNames(InAddedAssetData.AssetClass, AncestorClassNames);

			if (AncestorClassNames.Contains(UBlueprintCore::StaticClass()->GetFName()))
			{
				FString ClassObjectPath;
				if (InAddedAssetData.GetTagValue(FBlueprintTags::GeneratedClassPath, ClassObjectPath))
				{
					ClassObjectPath = FPackageName::ExportTextPathToObjectPath(ClassObjectPath);
				}

				// Make sure that the node does not already exist. There is a bit of double adding going on at times and this prevents it.
				if (!FindNodeByGeneratedClassPath(ObjectClassRoot, FName(*ClassObjectPath)).IsValid())
				{
					FClassFilterNodePtr NewNode;
					LoadUnloadedTagData(NewNode, InAddedAssetData);

					// Find the blueprint if it's loaded.
					FindClass(NewNode);

					// Resolve the parent's class name locally and use it to find the parent's class.
					FString ParentClassPath = NewNode->ParentClassPath.ToString();
					UClass* ParentClass = FindObject<UClass>(nullptr, *ParentClassPath);
					FClassFilterNodePtr ParentNode = FindParent(ObjectClassRoot, NewNode->ParentClassPath, ParentClass);
					if (ParentNode.IsValid())
					{
						ParentNode->AddChild(NewNode);

						// Make sure the children are properly sorted.
						SortChildren(ObjectClassRoot);

						// All Viewers must repopulate.
						ClassFilter::Helpers::RefreshAll();
					}
				}
			}
		}
	}

	FORCEINLINE bool SortClassFilterNodes(const FClassFilterNodePtr& A, const FClassFilterNodePtr& B)
	{
		check(A.IsValid());
		check(B.IsValid());

		// Pull out the FString, for ease of reading.
		const FString& AString = *A->GetClassName();
		const FString& BString = *B->GetClassName();

		return AString < BString;
	}

	void FClassHierarchy::SortChildren(FClassFilterNodePtr& InRootNode)
	{
		TArray< FClassFilterNodePtr >& ChildList = InRootNode->GetChildrenList();
		for (auto& Child : InRootNode->GetChildrenList())
		{
			// Setup the parent weak pointer, useful for going up the tree for unloaded blueprints.
			Child->ParentNode = InRootNode;

			// Check the child, then check the return to see if it is valid. If it is valid, end the recursion.
			SortChildren(Child);
		}

		// Sort the children.
		ChildList.Sort(SortClassFilterNodes);
	}

	void FClassHierarchy::FindClass(FClassFilterNodePtr InOutClassNode)
	{
		UClass* Class = FindObject<UClass>(nullptr, *InOutClassNode->ClassPath.ToString());

		if (Class)
		{
			InOutClassNode->Blueprint = Cast<UBlueprint>(Class->ClassGeneratedBy);
			InOutClassNode->Class = Class;
		}
	}

	void FClassHierarchy::LoadUnloadedTagData(FClassFilterNodePtr& InOutClassFilterNode, const FAssetData& InAssetData)
	{
		const FString ClassName = InAssetData.AssetName.ToString();
		FString ClassDisplayName = InAssetData.GetTagValueRef<FString>(FBlueprintTags::BlueprintDisplayName);
		if (ClassDisplayName.IsEmpty())
		{
			ClassDisplayName = ClassName;
		}
		// Create the viewer node. We use the name without _C for both
		InOutClassFilterNode = MakeShared<FClassFilterNode>(ClassName, ClassDisplayName);

		InOutClassFilterNode->BlueprintAssetPath = InAssetData.ObjectPath;

		FString ClassObjectPath;
		if (InAssetData.GetTagValue(FBlueprintTags::GeneratedClassPath, ClassObjectPath))
		{
			InOutClassFilterNode->ClassPath = FName(*FPackageName::ExportTextPathToObjectPath(ClassObjectPath));
		}

		FString ParentClassPathString;
		if (InAssetData.GetTagValue(FBlueprintTags::ParentClassPath, ParentClassPathString))
		{
			InOutClassFilterNode->ParentClassPath = FName(*FPackageName::ExportTextPathToObjectPath(ParentClassPathString));
		}

		InOutClassFilterNode->bIsBPNormalType = InAssetData.GetTagValueRef<FString>(FBlueprintTags::BlueprintType) == TEXT("BPType_Normal");

		// It is an unloaded blueprint, so we need to create the structure that will hold the data.
		TSharedPtr<FUnloadedBlueprintData> UnloadedBlueprintData = MakeShared<FUnloadedBlueprintData>(InOutClassFilterNode);
		InOutClassFilterNode->UnloadedBlueprintData = UnloadedBlueprintData;

		// Get the class flags.
		const uint32 ClassFlags = InAssetData.GetTagValueRef<uint32>(FBlueprintTags::ClassFlags);
		InOutClassFilterNode->UnloadedBlueprintData->SetClassFlags(ClassFlags);

		const FString ImplementedInterfaces = InAssetData.GetTagValueRef<FString>(FBlueprintTags::ImplementedInterfaces);
		if (!ImplementedInterfaces.IsEmpty())
		{
			FString FullInterface;
			FString RemainingString;
			FString InterfacePath;
			FString CurrentString = *ImplementedInterfaces;
			while (CurrentString.Split(TEXT(","), &FullInterface, &RemainingString))
			{
				if (!CurrentString.StartsWith(TEXT("Graphs=(")))
				{
					if (FullInterface.Split(TEXT("\""), &CurrentString, &InterfacePath, ESearchCase::CaseSensitive))
					{
						// The interface paths in metadata end with "', so remove those
						InterfacePath.RemoveFromEnd(TEXT("\"'"));

						FCoreRedirectObjectName ResolvedInterfaceName = FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Class, FCoreRedirectObjectName(InterfacePath));
						UnloadedBlueprintData->AddImplementedInterface(ResolvedInterfaceName.ObjectName.ToString());
					}
				}

				CurrentString = RemainingString;
			}
		}
	}

	void FClassHierarchy::PopulateClassHierarchy()
	{
		TArray< FClassFilterNodePtr > RootLevelClasses;

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

		// Retrieve all blueprint classes
		TArray<FAssetData> BlueprintList;

		FARFilter Filter;
		Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
		Filter.ClassNames.Add(UAnimBlueprint::StaticClass()->GetFName());
		Filter.ClassNames.Add(UBlueprintGeneratedClass::StaticClass()->GetFName());

		// Include any Blueprint based objects as well, this includes things like Blutilities, UMG, and GameplayAbility objects
		Filter.bRecursiveClasses = true;
		AssetRegistryModule.Get().GetAssets(Filter, BlueprintList);

		TMultiMap<FName, FAssetData> BlueprintPackageToAssetDataMap;
		for (int32 AssetIdx = 0; AssetIdx < BlueprintList.Num(); ++AssetIdx)
		{
			FClassFilterNodePtr NewNode;
			LoadUnloadedTagData(NewNode, BlueprintList[AssetIdx]);
			RootLevelClasses.Add(NewNode);

			// Find the blueprint if it's loaded.
			FindClass(NewNode);


			BlueprintPackageToAssetDataMap.Add(BlueprintList[AssetIdx].PackageName, BlueprintList[AssetIdx]);
		}

		AddChildren_NoFilter(ObjectClassRoot, BlueprintPackageToAssetDataMap);

		RootLevelClasses.Add(ObjectClassRoot);

		// Second pass to link them to parents.
		for (int32 CurrentNodeIdx = 0; CurrentNodeIdx < RootLevelClasses.Num(); ++CurrentNodeIdx)
		{
			if (RootLevelClasses[CurrentNodeIdx]->ParentClassPath != NAME_None)
			{
				// Resolve the parent's class name locally and use it to find the parent's class.
				FString ParentClassPath = RootLevelClasses[CurrentNodeIdx]->ParentClassPath.ToString();
				const UClass* ParentClass = FindObject<UClass>(nullptr, *ParentClassPath);

				for (int32 SearchNodeIdx = 0; SearchNodeIdx < RootLevelClasses.Num(); ++SearchNodeIdx)
				{
					FClassFilterNodePtr ParentNode = FindParent(RootLevelClasses[SearchNodeIdx], RootLevelClasses[CurrentNodeIdx]->ParentClassPath, ParentClass);
					if (ParentNode.IsValid())
					{
						// AddUniqueChild makes sure that when a node was generated one by EditorClassHierarchy and one from LoadUnloadedTagData - the proper one is selected
						ParentNode->AddUniqueChild(RootLevelClasses[CurrentNodeIdx]);
						RootLevelClasses.RemoveAtSwap(CurrentNodeIdx);
						--CurrentNodeIdx;
						break;
					}
				}

			}
		}

		// Recursively sort the children.
		SortChildren(ObjectClassRoot);

		// All viewers must refresh.
		ClassFilter::Helpers::RefreshAll();
	}
}
