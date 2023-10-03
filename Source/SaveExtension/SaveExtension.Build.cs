// Copyright 2015-2020 Piperift. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

namespace UnrealBuildTool.Rules
{

	public class SaveExtension : ModuleRules
	{
		public SaveExtension(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
			IWYUSupport = IWYUSupport.Full;

			PublicDependencyModuleNames.AddRange(new string[]
			{
			"Core",
			"Engine",
			"Foliage",
			"AIModule",
			"CoreUObject",
			"DeveloperSettings",
			"ImageWrapper",
			"NavigationSystem"
			});
		}
	}

}
