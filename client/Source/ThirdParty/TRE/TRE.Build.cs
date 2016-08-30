// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TRE : ModuleRules
{
    public TRE(TargetInfo Target)
    {
        //PublicDependencyModuleNames.AddRange(new string[] { "gl2ps" });

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

            string LibraryName = "TRE.lib";

            PublicAdditionalLibraries.Add(LibraryName);
            PublicAdditionalLibraries.Add("opengl32.lib");
        }

        Definitions.Add("NDEBUG;WIN32;_LIB;_TC_STATIC;_NO_BOOST");
    }
}
