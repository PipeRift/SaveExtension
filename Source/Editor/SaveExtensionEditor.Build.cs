// Copyright 2015-2020 Piperift. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class SaveExtensionEditor : ModuleRules
	{
		public SaveExtensionEditor(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
			IWYUSupport = IWYUSupport.Full;

			PublicDependencyModuleNames.AddRange(new string[]
			{
				"Core",
				"Engine",
				"CoreUObject",
				"Kismet",
				"SaveExtension"
			});

			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"AssetTools",
				"Projects",
				"InputCore",
				"UnrealEd",
				"SlateCore",
				"Slate",
				"EditorStyle",
				"ClassViewer",
				"BlueprintGraph",
				"GraphEditor",
				"PropertyEditor"
			});
		}
	}
}