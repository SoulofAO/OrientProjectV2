// Fill out your copyright notice in the Description page of Project Settings.

using System.Collections.Generic;
using UnrealBuildTool;

public class BNAModule : ModuleRules
{
	public BNAModule(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "XmlParser", "DeveloperSettings", "UnrealEd", "AssetTools"});

        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "EditorStyle", "UnrealEd", "LevelEditor", "WorkspaceMenuStructure", "PropertyEditor", "OrientProject", "AssetTools" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
