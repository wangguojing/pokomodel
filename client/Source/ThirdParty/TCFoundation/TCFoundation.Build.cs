// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TCFoundation : ModuleRules
{
    public TCFoundation(TargetInfo Target)
    {
        Type = ModuleType.External;

        bool isLibrarySupported = false;

        if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
        {
            isLibrarySupported = true;

            string CommonLibPath = ModuleDirectory + "/";
            string HeaderPath = CommonLibPath + "include/";

            PublicIncludePaths.Add(HeaderPath);

            string PlatformPath = (Target.Platform == UnrealTargetPlatform.Win64) ? "x64/" : "x86/";
            string ConfigPath = (Target.Configuration == UnrealTargetConfiguration.Development) ? "Release/" : "Debug/";

            //string LibraryPath = CommonLibPath + "Lib/" + PlatformPath + "VS" + WindowsPlatform.GetVisualStudioCompilerVersionName() + "/";
            string LibraryPath = CommonLibPath + "lib/";

            PublicLibraryPaths.Add(LibraryPath);

            //string LibraryName = "TCFoundation" + (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT ? "_d" : "") + ".lib";
            string LibraryName = "TCFoundation.lib";

            PublicAdditionalLibraries.Add(LibraryName);
        }

        Definitions.Add("_DEBUG;WIN32;_LIB;_BUILDING_TCFOUNDATION_LIB;_NO_BOOST;_VC80_UPGRADE=0x0600;_MBCS;");
    }
}
