// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LDLoader : ModuleRules
{
    public LDLoader(TargetInfo Target)
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

            string LibraryName = "LDLoader";

            //if (Target.Configuration != UnrealTargetConfiguration.Shipping)
            //{
            //    LibraryName += "_d";
            //}

            PublicAdditionalLibraries.Add(LibraryName + ".lib");
        }

        //Definitions.Add("WIN32;NDEBUG;_LIB;_TC_STATIC;_NO_BOOST;");
    }
}
