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
				"AIModule",
				"CoreUObject",
				"DeveloperSettings"
			});

			PrivateDependencyModuleNames.AddRange(new string[] { });

			if (Target.Type == TargetType.Editor)
			{
				PrivateDependencyModuleNames.AddRange(new string[]
				{
					"UnrealEd"
				});
			}
		}
	}

}
