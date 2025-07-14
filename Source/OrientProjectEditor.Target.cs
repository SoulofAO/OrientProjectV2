// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class OrientProjectEditorTarget : TargetRules
{
	public OrientProjectEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        bUseUnityBuild = false;
        bUsePCHFiles = false;
        ExtraModuleNames.AddRange( new string[] { "OrientProject", "BNAModule", "Magic", "TestMapPakFile" } );
	}
}
