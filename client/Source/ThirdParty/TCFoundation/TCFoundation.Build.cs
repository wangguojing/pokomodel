// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TCFoundation : ModuleRules
{
    public TCFoundation(TargetInfo Target)
    {
        Type = ModuleType.External;

        if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
        {
            //PrivateDependencyModuleNames.AddRange(new string[] { "UElibPNG" });

            string CommonLibPath = ModuleDirectory + "/";
            string HeaderPath = CommonLibPath;

            PublicIncludePaths.Add(HeaderPath + "../");

            string PlatformPath = (Target.Platform == UnrealTargetPlatform.Win64) ? "x64/" : "x86/";
            string ConfigPath = (Target.Configuration == UnrealTargetConfiguration.Development) ? "Release/" : "Debug/";
            string LibraryPath = CommonLibPath + "lib/";

            PublicLibraryPaths.Add(LibraryPath);

            string LibraryName = "TCFoundation.lib";

            PublicAdditionalLibraries.Add(LibraryName);
        }

        //Definitions.Add("NDEBUG;WIN32;_LIB;_WINSOCK_DEPRECATED_NO_WARNINGS;_BUILDING_TCFOUNDATION_LIB;NO_PNG_IMAGE_FORMAT;NO_JPG_IMAGE_FORMAT;_NO_BOOST");
    }
}
