// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class gl2ps : ModuleRules
{
    public gl2ps(TargetInfo Target)
    {
        //PrivateDependencyModuleNames.AddRange(new string[] { "zlib" });
        //PublicDependencyModuleNames.AddRange(new string[] { "zlib" });

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

            string LibraryName = "gl2ps.lib";

            PublicAdditionalLibraries.Add(LibraryName);
        }

        Definitions.Add("WIN32;NDEBUG;_LIB;_CRT_SECURE_NO_DEPRECATE;_USE_MATH_DEFINES;GL2PS_HAVE_ZLIB");
    }
}
