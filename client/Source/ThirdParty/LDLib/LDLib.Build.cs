// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LDLib : ModuleRules
{
    public LDLib(TargetInfo Target)
    {

        Type = ModuleType.External;

        if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
        {
            string CommonLibPath = ModuleDirectory + "/";
            string HeaderPath = CommonLibPath;

            PublicIncludePaths.Add(HeaderPath + "../");

            string PlatformPath = (Target.Platform == UnrealTargetPlatform.Win64) ? "x64/" : "x86/";
            string ConfigPath = (Target.Configuration == UnrealTargetConfiguration.Development) ? "Release/" : "Debug/";
            string LibraryPath = CommonLibPath + "lib/";

            PublicLibraryPaths.Add(LibraryPath);

            string LibraryName = "LDLib.lib";

            PublicAdditionalLibraries.Add(LibraryName);
        }

        //PrivateDependencyModuleNames.AddRange(new string[] { "LDExporter", "TCFoundation", "LDLoader", "TRE" });

        //Definitions.Add("NDEBUG;WIN32;_LIB;_BUILDING_TCFOUNDATION_LIB;NO_JPG_IMAGE_FORMAT");
    }
}
