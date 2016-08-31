// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class client : ModuleRules
{
	public client(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore",
        "Core", "CoreUObject", "Engine", "InputCore", "ProceduralMeshComponent" });

		PrivateDependencyModuleNames.AddRange(new string[] { "LDLib" });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");
        // if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
        // {
        //		if (UEBuildConfiguration.bCompileSteamOSS == true)
        //		{
        //			DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");
        //		}
        // }

        Definitions.Add("NDEBUG");
        Definitions.Add("WIN32");
        Definitions.Add("_WINDOWS");
        Definitions.Add("_TC_STATIC");
        Definitions.Add("LDVIEW_APP");
        Definitions.Add("_NO_BOOST");
    }
}
