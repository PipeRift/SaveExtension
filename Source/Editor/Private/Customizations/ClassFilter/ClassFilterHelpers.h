// Copyright 2015-2019 Piperift. All Rights Reserved.
#pragma once

#include "ClassFilterNode.h"
#include <AssetData.h>
#include <Engine/Blueprint.h>
#include <Kismet2/KismetEditorUtilities.h>
#include <CoreGlobals.h>
#include <Misc/MessageDialog.h>
#include <PackageTools.h>
#include <Toolkits/AssetEditorManager.h>
#include <AssetRegistryModule.h>
#include <EditorDirectories.h>
#include <Dialogs/Dialogs.h>
#include <AssetToolsModule.h>
#include <IContentBrowserSingleton.h>
#include <ContentBrowserModule.h>
#include <GameProjectGeneration/Public/GameProjectGenerationModule.h>
#include <Logging/MessageLog.h>
#include <SourceCodeNavigation.h>
#include <Framework/Multibox/MultiBoxBuilder.h>
#include <Misc/FeedbackContext.h>
#include <Misc/ConfigCacheIni.h>
#include "Misc/ClassFilter.h"

#define LOCTEXT_NAMESPACE "ClassFilterHelpers"


/* Based on ClassViewer private logic.
 * Believe me, I would have loved to share code
 */

namespace ClassFilter
{
	class FClassHierarchy
	{
	private:
		/** The "Object" class node that is used as a rooting point for the Class Viewer. */
		FClassFilterNodePtr ObjectClassRoot;

		/** Handles to various registered RequestPopulateClassHierarchy delegates */
		FDelegateHandle OnFilesLoadedRequestPopulateClassHierarchyDelegateHandle;
		FDelegateHandle OnBlueprintCompiledRequestPopulateClassHierarchyDelegateHandle;
		FDelegateHandle OnClassPackageLoadedOrUnloadedRequestPopulateClassHierarchyDelegateHandle;


	public:
		FClassHierarchy();
		~FClassHierarchy();

		/** Populates the class hierarchy tree, pulling all the loaded and unloaded classes into a master tree. */
		void PopulateClassHierarchy();

		/** Recursive function to sort a tree.
		 *	@param InOutRootNode						The current node to sort.
		 */
		void SortChildren(FClassFilterNodePtr& InRootNode);

		/**
		 *	@return The ObjectClassRoot for building a duplicate tree using.
		 */
		const FClassFilterNodePtr GetObjectRootNode() const
		{
			// This node should always be valid.
			check(ObjectClassRoot.IsValid())

			return ObjectClassRoot;
		}

		/** Finds the parent of a node, recursively going deeper into the hierarchy.
		 *	@param InRootNode							The current class node to examine.
		 *	@param InParentClassName					The classname to look for.
		 *	@param InParentClass						The parent class to look for.
		 *
		 *	@return The parent node.
		 */
		FClassFilterNodePtr FindParent(const FClassFilterNodePtr& InRootNode, FName InParentClassname, const UClass* InParentClass);

		/** Updates the Class of a node. Uses the generated class package name to find the node.
		 *	@param InGeneratedClassPath		The path of the generated class to find the node for.
		 *	@param InNewClass				The class to update the node with.
		*/
		void UpdateClassInNode(FName InGeneratedClassPath, UClass* InNewClass, UBlueprint* InNewBluePrint);

		/** Finds the node, recursively going deeper into the hierarchy. Does so by comparing class names.
		*	@param InClassName			The name of the generated class package to find the node for.
		*
		*	@return The node.
		*/
		FClassFilterNodePtr FindNodeByClassName(const FClassFilterNodePtr& InRootNode, const FString& InClassName);

		/** Finds the node, recursively going deeper into the hierarchy. Does so by comparing class names.
		*	@param InClass The pointer of the class to find the node for.
		*
		*	@return The node.
		*/
		FClassFilterNodePtr FindNodeByClass(const FClassFilterNodePtr& InRootNode, const UClass* Class);


	private:
		/** Recursive function to build a tree, will not filter.
		 *	@param InOutRootNode						The node that this function will add the children of to the tree.
		 *  @param PackageNameToAssetDataMap			The asset registry map of blueprint package names to blueprint data
		 */
		void AddChildren_NoFilter(FClassFilterNodePtr& InOutRootNode, const TMultiMap<FName, FAssetData>& BlueprintPackageToAssetDataMap);

		/** Called when hot reload has finished */
		void OnHotReload(bool bWasTriggeredAutomatically);

		/** Finds the node, recursively going deeper into the hierarchy. Does so by comparing generated class package names.
		 *	@param InGeneratedClassPath		The path of the generated class to find the node for.
		 *
		 *	@return The node.
		 */
		FClassFilterNodePtr FindNodeByGeneratedClassPath(const FClassFilterNodePtr& InRootNode, FName InGeneratedClassPath);

		/**
		 * Loads the tag data for an unloaded blueprint asset.
		 *
		 * @param InOutClassFilterNode		The node to save all the data into.
		 * @param InAssetData				The asset data to pull the tags from.
		 */
		void LoadUnloadedTagData(FClassFilterNodePtr& InOutClassFilterNode, const FAssetData& InAssetData);

		/**
		 * Finds the UClass and UBlueprint for the passed in node, utilizing unloaded data to find it.
		 *
		 * @param InOutClassNode		The node to find the class and fill out.
		 */
		void FindClass(FClassFilterNodePtr InOutClassNode);

		/**
		 * Recursively searches through the hierarchy to find and remove the asset. Used when deleting assets.
		 *
		 * @param InRootNode	The node to start the search with.
		 * @param InClassPath	The class path of the asset to delete
		 *
		 * @return Returns true if the asset was found and deleted successfully.
		 */
		bool FindAndRemoveNodeByClassPath(const FClassFilterNodePtr& InRootNode, FName InClassPath);

		/** Callback registered to the Asset Registry to be notified when an asset is added. */
		void AddAsset(const FAssetData& InAddedAssetData);

		/** Callback registered to the Asset Registry to be notified when an asset is removed. */
		void RemoveAsset(const FAssetData& InRemovedAssetData);
	};


	namespace Helpers
	{
		DECLARE_MULTICAST_DELEGATE( FPopulateClassFilter );

		/** The class hierarchy that manages the unfiltered class tree for the Class Viewer. */
		static TSharedPtr< FClassHierarchy > ClassHierarchy;

		/** Used to inform any registered Class Viewers to refresh. */
		static FPopulateClassFilter PopulateClassFilterDelegate;

		/** true if the Class Hierarchy should be populated. */
		static bool bPopulateClassHierarchy;

		// Pre-declare these functions.
		static bool CheckIfBlueprintBase( FClassFilterNodePtr InNode );
		static UBlueprint* GetBlueprint( UClass* InClass );
		static void UpdateClassInNode(FName InGeneratedClassPath, UClass* InNewClass, UBlueprint* InNewBluePrint );

		/** Util class to checks if a particular class can be made into a Blueprint, ignores deprecation
		 *
		 * @param InClass					The class to verify can be made into a Blueprint
		 * @return							TRUE if the class can be made into a Blueprint
		 */
		static bool CanCreateBlueprintOfClass_IgnoreDeprecation(UClass* InClass)
		{
			// Temporarily remove the deprecated flag so we can check if it is valid for
			bool bIsClassDeprecated = InClass->HasAnyClassFlags(CLASS_Deprecated);
			InClass->ClassFlags &= ~CLASS_Deprecated;

			bool bCanCreateBlueprintOfClass = FKismetEditorUtilities::CanCreateBlueprintOfClass( InClass );

			// Reassign the deprecated flag if it was previously assigned
			if(bIsClassDeprecated)
			{
				InClass->ClassFlags |= CLASS_Deprecated;
			}

			return bCanCreateBlueprintOfClass;
		}

		/** Checks if a particular class is abstract.
		 *	@param InClass				The Class to check.
		 *	@return Returns true if the class is abstract.
		 */
		static bool IsAbstract(const UClass* InClass)
		{
			return InClass->HasAnyClassFlags(CLASS_Abstract);
		}

		/** Checks if the TestString passes the filter.
		 *	@param InTestString			The string to test against the filter.
		 *
		 *	@return	true if it passes the filter.
		 */
		static bool PassesFilter(const FString& InTestString)
		{
			return true;
		}

		/** Will create the instance of FClassHierarchy and populate the class hierarchy tree. */
		static void ConstructClassHierarchy()
		{
			if(!ClassHierarchy.IsValid())
			{
				ClassHierarchy = MakeShared<FClassHierarchy>();

				// When created, populate the hierarchy.
				GWarn->BeginSlowTask( LOCTEXT("RebuildingClassHierarchy", "Rebuilding Class Hierarchy"), true );
				ClassHierarchy->PopulateClassHierarchy();
				GWarn->EndSlowTask();
			}
		}

		/** Cleans up the Class Hierarchy */
		static void DestroyClassHierachy()
		{
			ClassHierarchy.Reset();
		}

		/** Will populate the class hierarchy tree if previously requested. */
		static void PopulateClassHierarchy()
		{
			if(bPopulateClassHierarchy)
			{
				bPopulateClassHierarchy = false;

				GWarn->BeginSlowTask( LOCTEXT("RebuildingClassHierarchy", "Rebuilding Class Hierarchy"), true );
				ClassHierarchy->PopulateClassHierarchy();
				GWarn->EndSlowTask();
			}
		}

		/** Will enable the Class Hierarchy to be populated next Tick. */
		static void RequestPopulateClassHierarchy()
		{
			bPopulateClassHierarchy = true;
		}

		/** Refreshes all registered instances of Class Viewer/Pickers. */
		static void RefreshAll()
		{
			ClassFilter::Helpers::PopulateClassFilterDelegate.Broadcast();
		}

		/** Recursive function to build a tree, filtering out nodes based on the InitOptions and filter search terms.
		 *	@param InInitOptions						The class viewer's options, holds the AllowedClasses and DisallowedClasses.
		 *	@param InOutRootNode						The node that this function will add the children of to the tree.
		 *	@param InRootClassIndex						The index of the root node.
		 *	@param bInOnlyBlueprintBases				Filter option to remove non-blueprint base classes.
		 *	@param bInShowUnloadedBlueprints			Filter option to not remove unloaded blueprints due to class filter options.
		 *  @param bInInternalClasses                   Filter option for showing internal classes.
		 *  @param InternalClasses                      The classes that have been marked as Internal Only.
		 *  @param InternalPaths                        The paths that have been marked Internal Only.
		 *
		 *	@return Returns true if the child passed the filter.
		 */
		static bool AddChildren_Tree(const FClassFilter& Filter, FClassFilterNodePtr& InOutRootNode,
			const FClassFilterNodePtr& InOriginalRootNode,
			bool bInOnlyBlueprintBases, bool bInShowUnloadedBlueprints, bool bInInternalClasses,
			const TArray<UClass*>& InternalClasses, const TArray<FDirectoryPath>& InternalPaths)
		{
			bool bChildrenPassesFilter= false;
			bool bReturnPassesFilter = false;

			bool bPassesBlueprintBaseFilter = !bInOnlyBlueprintBases || CheckIfBlueprintBase(InOriginalRootNode);
			bool bIsUnloadedBlueprint = !InOriginalRootNode->Class.IsValid();

			FString GeneratedClassPathString = InOriginalRootNode->ClassPath.ToString();

			// The INI files declare classes and folders that are considered internal only. Does this class match any of those patterns?
			// INI path: /Script/ClassFilter.ClassFilterProjectSettings
			bool bPassesInternalFilter = true;
			if (!bInInternalClasses && InternalPaths.Num() > 0)
			{
				for (int i = 0; i < InternalPaths.Num(); i++)
				{
					if (GeneratedClassPathString.StartsWith(InternalPaths[i].Path))
					{
						bPassesInternalFilter = false;
						break;
					}
				}
			}
			if (!bInInternalClasses && InternalClasses.Num() > 0 && bPassesInternalFilter && InOriginalRootNode->Class.IsValid())
			{
				for (int i = 0; i < InternalClasses.Num(); i++)
				{
					if (InOriginalRootNode->Class->IsChildOf(InternalClasses[i]))
					{
						bPassesInternalFilter = false;
						break;
					}
				}
			}

			// There are few options for filtering an unloaded blueprint, if it matches with this filter, it passes.
			if(bIsUnloadedBlueprint)
			{
				if(bInShowUnloadedBlueprints)
				{
					bReturnPassesFilter = InOutRootNode->bPassesFilter = bPassesBlueprintBaseFilter && bPassesInternalFilter && PassesFilter(*InOriginalRootNode->GetClassName());
				}
			}
			else
			{
				bReturnPassesFilter = InOutRootNode->bPassesFilter = bPassesBlueprintBaseFilter && bPassesInternalFilter && PassesFilter(*InOriginalRootNode->GetClassName());
			}

			for(const auto& Child : InOriginalRootNode->GetChildrenList())
			{
				FClassFilterNodePtr NewNode = MakeShared<FClassFilterNode>( *Child.Get() );

				NewNode->SetStateFromFilter(Filter);

				bChildrenPassesFilter = AddChildren_Tree(Filter, NewNode, Child, bInOnlyBlueprintBases, bInShowUnloadedBlueprints, bInInternalClasses, InternalClasses, InternalPaths);
				if(bChildrenPassesFilter)
				{
					InOutRootNode->AddChild(NewNode);
				}
				bReturnPassesFilter |= bChildrenPassesFilter;
			}
			return bReturnPassesFilter;
		}

		/** Builds the class tree.
		 *	@param InInitOptions						The class viewer's options, holds the AllowedClasses and DisallowedClasses.
		 *	@param InOutRootNode						The node to root the tree to.
		 *	@param bInOnlyBlueprintBases				Filter option to remove non-blueprint base classes.
		 *	@param bInShowUnloadedBlueprints			Filter option to not remove unloaded blueprints due to class filter options.
		 *  @param bInInternalClasses                   Filter option for showing internal classes.
		 *  @param InternalClasses                      The classes that have been marked as Internal Only.
		 *  @param InternalPaths                        The paths that have been marked Internal Only.
		 *	@return A fully built tree.
		 */
		static void GetClassTree(const FClassFilter& Filter, FClassFilterNodePtr& InOutRootNode, bool bInOnlyBlueprintBases,
			bool bInShowUnloadedBlueprints, bool bInInternalClasses = true,
			const TArray<UClass*>& InternalClasses = TArray<UClass*>(), const TArray<FDirectoryPath>& InternalPaths = TArray<FDirectoryPath>())
		{
			// Use BaseClass as root
			FClassFilterNodePtr RootNode;
			if (Filter.GetBaseClass())
			{
				RootNode = ClassHierarchy->FindNodeByClass(ClassHierarchy->GetObjectRootNode(), Filter.GetBaseClass());
			}
			else // Use UObject as root
			{
				RootNode = ClassHierarchy->GetObjectRootNode();
			}

			// Duplicate the node, it will have no children.
			InOutRootNode = MakeShared<FClassFilterNode>(*RootNode);

			AddChildren_Tree(Filter, InOutRootNode, RootNode, bInOnlyBlueprintBases, bInShowUnloadedBlueprints, bInInternalClasses, InternalClasses, InternalPaths);
		}

		/** Recursive function to build the list, filtering out nodes based on the InitOptions and filter search terms.
		 *	@param InInitOptions						The class viewer's options, holds the AllowedClasses and DisallowedClasses.
		 *	@param InOutRootNode						The node that this function will add the children of to the tree.
		 *	@param InRootClassIndex						The index of the root node.
		 *	@param bInOnlyBlueprintBases				Filter option to remove non-blueprint base classes.
		 *	@param bInShowUnloadedBlueprints			Filter option to not remove unloaded blueprints due to class filter options.
		 *  @param bInInternalClasses                   Filter option for showing internal classes.
		 *  @param InternalClasses                      The classes that have been marked as Internal Only.
		 *  @param InternalPaths                        The paths that have been marked Internal Only.
		 *
		 *	@return Returns true if the child passed the filter.
		 */
		static void AddChildren_List(TArray< FClassFilterNodePtr >& InOutNodeList,
			const FClassFilterNodePtr& InOriginalRootNode,
			bool bInOnlyBlueprintBases, bool bInShowUnloadedBlueprints,
			bool bInInternalClasses,
			const TArray<UClass*>& InternalClasses, const TArray<FDirectoryPath>& InternalPaths)
		{
			bool bPassesBlueprintBaseFilter = !bInOnlyBlueprintBases || CheckIfBlueprintBase(InOriginalRootNode);
			bool bIsUnloadedBlueprint = !InOriginalRootNode->Class.IsValid();

			FString GeneratedClassPathString = InOriginalRootNode->ClassPath.ToString();

			// The INI files declare classes and folders that are considered internal only. Does this class match any of those patterns?
			// INI path: /Script/ClassFilter.ClassFilterProjectSettings
			bool bPassesInternalFilter = true;
			if (!bInInternalClasses && InternalPaths.Num() > 0)
			{
				for (int i = 0; i < InternalPaths.Num(); i++)
				{
					if (GeneratedClassPathString.StartsWith(InternalPaths[i].Path))
					{
						bPassesInternalFilter = false;
						break;
					}
				}
			}
			if (!bInInternalClasses && InternalClasses.Num() > 0 && bPassesInternalFilter)
			{
				for (int i = 0; i < InternalClasses.Num(); i++)
				{
					if (InOriginalRootNode->Class->IsChildOf(InternalClasses[i]))
					{
						bPassesInternalFilter = false;
						break;
					}
				}
			}

			FClassFilterNodePtr NewNode = MakeShared<FClassFilterNode>(*InOriginalRootNode.Get());

			// There are few options for filtering an unloaded blueprint, if it matches with this filter, it passes.
			if(bIsUnloadedBlueprint)
			{
				if(bInShowUnloadedBlueprints)
				{
					NewNode->bPassesFilter = bPassesBlueprintBaseFilter && bPassesInternalFilter && PassesFilter(*InOriginalRootNode->GetClassName());
				}
			}
			else
			{
				NewNode->bPassesFilter = bPassesBlueprintBaseFilter && bPassesInternalFilter && PassesFilter(*InOriginalRootNode->GetClassName());
			}

			if(NewNode->bPassesFilter)
			{
				InOutNodeList.Add(NewNode);
			}

			for (const auto& Child : InOriginalRootNode->GetChildrenList())
			{
				AddChildren_List(InOutNodeList, Child, bInOnlyBlueprintBases,
					bInShowUnloadedBlueprints, bInInternalClasses, InternalClasses, InternalPaths);
			}
		}

		/** Builds the class list.
		 *	@param InInitOptions						The class viewer's options, holds the AllowedClasses and DisallowedClasses.
		 *	@param InOutNodeList						The list to add all the nodes to.
		 *	@param bInOnlyBlueprintBases				Filter option to remove non-blueprint base classes.
		 *	@param bInShowUnloadedBlueprints			Filter option to not remove unloaded blueprints due to class filter options.
		 *  @param bInInternalClasses                   Filter option for showing internal classes.
		 *  @param InternalClasses                      The classes that have been marked as Internal Only.
		 *  @param InternalPaths                        The paths that have been marked Internal Only.
		 *
		 *	@return A fully built list.
		 */
		static void GetClassList(TArray< FClassFilterNodePtr >& InOutNodeList,
			bool bInOnlyBlueprintBases, bool bInShowUnloadedBlueprints,
			bool bInInternalClasses = true,
			const TArray<UClass*>& InternalClasses = TArray<UClass*>(), const TArray<FDirectoryPath>& InternalPaths = TArray<FDirectoryPath>())
		{
			const FClassFilterNodePtr ObjectClassRoot = ClassHierarchy->GetObjectRootNode();

			for (const auto& Child : ObjectClassRoot->GetChildrenList())
			{
				AddChildren_List(InOutNodeList, Child, bInOnlyBlueprintBases, bInShowUnloadedBlueprints, bInInternalClasses, InternalClasses, InternalPaths);
			}
		}

		/** Retrieves the blueprint for a class index.
		 *	@param InClass							The class whose blueprint is desired.
		 *
		 *	@return									The blueprint associated with the class index.
		 */
		static UBlueprint* GetBlueprint( UClass* InClass )
		{
			if( InClass->ClassGeneratedBy && InClass->ClassGeneratedBy->IsA(UBlueprint::StaticClass()) )
			{
				return Cast<UBlueprint>(InClass->ClassGeneratedBy);
			}

			return nullptr;
		}

		/** Retrieves a few items of information on the given UClass (retrieved via the InClassIndex).
		 *	@param InClass							The class to gather info of.
		 *	@param bInOutIsBlueprintBase			true if the class is a blueprint.
		 *	@param bInOutHasBlueprint				true if the class has a blueprint.
		 *
		 *	@return									The blueprint associated with the class index.
		 */
		static void GetClassInfo( TWeakObjectPtr<UClass> InClass, bool& bInOutIsBlueprintBase, bool& bInOutHasBlueprint )
		{
			if (UClass* Class = InClass.Get())
			{
				bInOutIsBlueprintBase = CanCreateBlueprintOfClass_IgnoreDeprecation( Class );
				bInOutHasBlueprint = Class->ClassGeneratedBy != nullptr;
			}
			else
			{
				bInOutIsBlueprintBase = false;
				bInOutHasBlueprint = false;
			}
		}

		/** Checks if a node is a blueprint base or not.
		 *	@param	InNode					The node to check if it is a blueprint base.
		 *
		 *	@return							true if the class is a blueprint.
		 */
		static bool CheckIfBlueprintBase( TSharedPtr< FClassFilterNode> InNode )
		{
			// If there is no class, it may be an unloaded blueprint.
			if(UClass* Class = InNode->Class.Get())
			{
				return CanCreateBlueprintOfClass_IgnoreDeprecation(Class);
			}
			else if(InNode->bIsBPNormalType)
			{
				bool bAllowDerivedBlueprints = false;
				GConfig->GetBool(TEXT("Kismet"), TEXT("AllowDerivedBlueprints"), /*out*/ bAllowDerivedBlueprints, GEngineIni);

				return bAllowDerivedBlueprints;
			}

			return false;
		}

		/**
		 * Creates a blueprint from a class.
		 *
		 * @param	InOutClassNode			Class node to pull what class to load and to update information in.
		 */
		static void LoadClass(FClassFilterNodePtr InOutClassNode)
		{
			GWarn->BeginSlowTask(LOCTEXT("LoadPackage", "Loading Package..."), true);
			UClass* Class = LoadObject<UClass>(nullptr, *InOutClassNode->ClassPath.ToString());
			GWarn->EndSlowTask();

			if (Class)
			{
				InOutClassNode->Blueprint = Cast<UBlueprint>(Class->ClassGeneratedBy);
				InOutClassNode->Class = Class;

				// Tell the original node to update so when a refresh happens it will still know about the newly loaded class.
				ClassFilter::Helpers::UpdateClassInNode(InOutClassNode->ClassPath, InOutClassNode->Class.Get(), InOutClassNode->Blueprint.Get() );
			}
			else
			{
				FMessageLog EditorErrors("EditorErrors");
				FFormatNamedArguments Arguments;
				Arguments.Add(TEXT("ObjectName"), FText::FromName(InOutClassNode->ClassPath));
				EditorErrors.Error(FText::Format(LOCTEXT("PackageLoadFail", "Failed to load class {ObjectName}"), Arguments));
			}
		}

		/** Updates the Class of a node. Uses the generated class package name to find the node.
		*	@param InGeneratedClassPath			The name of the generated class to find the node for.
		*	@param InNewClass					The class to update the node with.
		*/
		static void UpdateClassInNode(FName InGeneratedClassPath, UClass* InNewClass, UBlueprint* InNewBluePrint )
		{
			ClassHierarchy->UpdateClassInNode(InGeneratedClassPath, InNewClass, InNewBluePrint );
		}
	} // namespace Helpers
} // namespace ClassFilter

#undef LOCTEXT_NAMESPACE
