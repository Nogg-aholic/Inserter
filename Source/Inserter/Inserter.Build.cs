// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using System;
using System.Runtime.InteropServices;
using System.Text;
using Tools.DotNETCommon;

public class Inserter : ModuleRules
{
	public Inserter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        var factoryGamePchPath = new DirectoryReference(Path.Combine(Target.ProjectFile.Directory.ToString(), "Source", "FactoryGame", "Public", "FactoryGame.h"));
        PrivatePCHHeaderFile = factoryGamePchPath.MakeRelativeTo(new DirectoryReference(ModuleDirectory));
        PublicDependencyModuleNames.AddRange(
			new string[]
			{
			"Core", "CoreUObject",
            "Engine",
            "AnimGraphRuntime",
			"FactoryGame", "SML"
				// ... add other public dependencies that you statically link with here ...
			}
			);
        if (Target.Type == TargetRules.TargetType.Editor)
        {
            PublicDependencyModuleNames.AddRange(new string[] { "OnlineBlueprintSupport", "AnimGraph" });
        }

	}
}
