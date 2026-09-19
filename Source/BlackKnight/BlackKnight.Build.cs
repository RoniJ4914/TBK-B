// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BlackKnight : ModuleRules
{
	public BlackKnight(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"GameplayTasks",
			"NavigationSystem",
			"Niagara",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore",
			"GameplayTags",
			"ALS",
			"ALSCamera"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		if (Target.bBuildEditor)
		{
			// Editor-only: backs the content-generating commandlets in
			// Source/BlackKnight/Editor. Guarded by WITH_EDITOR in code, so a
			// Game-target build never needs these.
			PrivateDependencyModuleNames.Add("UnrealEd");
			PrivateDependencyModuleNames.Add("AssetRegistry");
			PrivateDependencyModuleNames.Add("AnimationCore");
		}

		PublicIncludePaths.AddRange(new string[] {
			"BlackKnight",
			"BlackKnight/AI",
			"BlackKnight/Core",
			"BlackKnight/Character",
			"BlackKnight/Combat",
			"BlackKnight/Combat/Animation",
			"BlackKnight/Combat/Interfaces",
			"BlackKnight/Editor",
			"BlackKnight/UI",
			"BlackKnight/Variant_Platforming",
			"BlackKnight/Variant_Platforming/Animation",
			"BlackKnight/Variant_Combat",
			"BlackKnight/Variant_Combat/AI",
			"BlackKnight/Variant_Combat/Animation",
			"BlackKnight/Variant_Combat/Gameplay",
			"BlackKnight/Variant_Combat/Interfaces",
			"BlackKnight/Variant_Combat/UI",
			"BlackKnight/Variant_SideScrolling",
			"BlackKnight/Variant_SideScrolling/AI",
			"BlackKnight/Variant_SideScrolling/Gameplay",
			"BlackKnight/Variant_SideScrolling/Interfaces",
			"BlackKnight/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
