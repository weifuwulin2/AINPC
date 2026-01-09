// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AINPC : ModuleRules
{
	public AINPC(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			
			// 新增的模块：
			"HTTP", 
			"Json", 
			"JsonUtilities",
			"NavigationSystem",
			"GameplayTags"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"AINPC",
			"AINPC/Variant_Platforming",
			"AINPC/Variant_Platforming/Animation",
			"AINPC/Variant_Combat",
			"AINPC/Variant_Combat/AI",
			"AINPC/Variant_Combat/Animation",
			"AINPC/Variant_Combat/Gameplay",
			"AINPC/Variant_Combat/Interfaces",
			"AINPC/Variant_Combat/UI",
			"AINPC/Variant_SideScrolling",
			"AINPC/Variant_SideScrolling/AI",
			"AINPC/Variant_SideScrolling/Gameplay",
			"AINPC/Variant_SideScrolling/Interfaces",
			"AINPC/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
