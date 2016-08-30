// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LDLib : ModuleRules
{
    public LDLib(TargetInfo Target)
    {
        PrivateDependencyModuleNames.AddRange(new string[] { "gl2ps", "tinyxml", "zlib", 
            "LDExporter", "TCFoundation", "LDLoader", "TRE" });

        Type = ModuleType.External;

        if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
        {
            string HeaderPath = ModuleDirectory + "/";

            PublicIncludePaths.Add(HeaderPath + "../");

            string PlatformPath = (Target.Platform == UnrealTargetPlatform.Win64) ? "x64/" : "x86/";
            string ConfigPath = (Target.Configuration == UnrealTargetConfiguration.Development) ? "Release/" : "Debug/";
            string LibraryPath = HeaderPath + "lib/";

            PublicLibraryPaths.Add(LibraryPath);

            PublicAdditionalLibraries.Add("LDLib.lib");
            PublicAdditionalLibraries.Add("glu32.lib");
        }

        Definitions.Add("NDEBUG;WIN32;_LIB;_TC_STATIC;_NO_BOOST");
    }
}
