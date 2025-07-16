/*
* Copyright 2025 DungeonStation, All Rights Reserved.
*/

using UnrealBuildTool;

public class AFSEditor : ModuleRules
{
    public AFSEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "AssetTools" 
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "AFS",
            "UnrealEd",
            "Slate",
            "SlateCore",
            "AssetTools",
            "AssetRegistry",
            "EditorStyle",
            "PropertyEditor",
            "InputCore",    
            "RenderCore",   
            "RHI",          
            "AdvancedPreviewScene",
             "EditorFramework",
             "ToolMenus"
        });
    }
}
