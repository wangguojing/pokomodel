; LDView setup script.
; NOTE: this script requires Inno Setup Preprocessor (ISPP).
; To download and install ISPP, get the Inno Setup QuickStart Pack from:
; http://www.jrsoftware.org/isdl.php#qsp

#define AppFilename 'Build\Release\LDView.exe'
#define SetupFilename(str Filename) ParseVersion(Filename, Local[0], Local[1], Local[2], Local[3]), "LDView-" + Str(Local[0]) + Str(Local[1]) + (Local[2] < 10 ? '0' : '') + Str(Local[2])

#define AppVerName GetFileDescription(AppFilename) + ' ' + GetFileVersionString(AppFilename)
#define IntallDescription  AppVerName + ' Setup'
#define AppVersion GetFileVersion(AppFilename)

[Setup]
AppName=LDView
AppVerName={#AppVerName}
AppVersion={#AppVersion}
VersionInfoVersion={#AppVersion}
VersionInfoDescription={#IntallDescription}
VersionInfoCopyright={#GetFileCopyright(AppFilename)}
OutputBaseFilename={#SetupFilename(AppFilename)}
SourceDir=.
AppPublisher=Travis Cobbs
AppPublisherURL=http://ldview.sourceforge.net/
AppSupportURL=http://ldview.sourceforge.net/
AppUpdatesURL=http://ldview.sourceforge.net/
Compression=lzma
DefaultDirName={pf}\LDView
DefaultGroupName=LDView
OutputDir=Setup
AllowNoIcons=yes
LicenseFile=License.txt
SetupIconFile=Icons\LDViewIcon.ico
ChangesAssociations=yes

[Code]
function IsX64: Boolean;
begin
  Result := IsWin64 and (ProcessorArchitecture = paX64);
end;

[Tasks]
Name: desktopicon; Description: Create a &desktop icon; GroupDescription: Additional icons:; Flags: unchecked
Name: quicklaunchicon; Description: Create a &Quick Launch icon; GroupDescription: Additional icons:; Flags: unchecked
Name: screensaver; Description: Install LDView as a screen saver; GroupDescription: Screen Saver:
Name: registerfiles; Description: Use LDView to open LDraw models; GroupDescription: File Associations:; Flags: unchecked
Name: thumbnails; Description: Use LDView to generate thumbnails in Windows Explorer; GroupDescription: Explorer Thumbnails:; Flags: unchecked
Name: prefsets; Description: Install preset Preference Sets; GroupDescription: Preference Sets:

[Files]
Source: Build\Release\LDView.exe; DestDir: {app}; Flags: ignoreversion
Source: license.txt; DestDir: {app}; Flags: ignoreversion
; NOTE: The first Readme.txt is for everything prior to Windows Vista, and can
;       be shown at the end of the installation.  However, since in Vista that
;       would launch Notepad with the same security permissions as the
;       installer, we won't let the user view it from the installer there.
Source: Readme.txt; DestDir: {app}; Flags: ignoreversion isreadme; OnlyBelowVersion: 0,6
Source: Readme.txt; DestDir: {app}; Flags: ignoreversion; MinVersion: 0,6
Source: ChangeHistory.html; DestDir: {app}; Flags: ignoreversion
Source: Help.html; DestDir: {app}; Flags: ignoreversion
Source: m6459.ldr; DestDir: {app}; Flags: ignoreversion
Source: 8464.mpd; DestDir: {app}; Flags: ignoreversion
Source: LDExporter\LGEO.xml; DestDir: {app}; Flags: ignoreversion
Source: Help\LDView.hlp; DestDir: {app}; Flags: ignoreversion
Source: LDView Home Page.url; DestDir: {app}; Flags: ignoreversion
Source: Build\Release\LDView.exe; DestDir: {sys}; DestName: LDView.scr; Flags: ignoreversion restartreplace; Tasks: screensaver
Source: Build\Release\LDViewThumbs.dll; DestDir: {app}; Flags: 32bit ignoreversion restartreplace regserver uninsrestartdelete; Tasks: thumbnails
Source: Build\Release64\LDViewThumbs64.dll; DestDir: {app}; Flags: 64bit ignoreversion restartreplace regserver noregerror uninsrestartdelete; Tasks: thumbnails; Check: IsX64
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: {group}\LDView; Filename: {app}\LDView.exe
Name: {group}\LDView Home Page; Filename: {app}\LDView Home Page.url
Name: {group}\Uninstall LDView; Filename: {uninstallexe}
Name: {commondesktop}\LDView; Filename: {app}\LDView.exe; Tasks: desktopicon
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\LDView; Filename: {app}\LDView.exe; Tasks: quicklaunchicon

[Run]
; NOTE: The OnlyBelowVersion flag below prevents this from showing up in Windows
;       Vista.  If it did show up in Vista, LDView would be executed with the
;       same security permissions as the installer, and that's considered to be
;       a bad thing there.
Filename: {app}\LDView.exe; Parameters: """{app}\m6459.ldr"""; Description: Launch LDView; Flags: nowait postinstall skipifsilent

[Registry]
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\LDView.exe; ValueType: string; ValueName: ; ValueData: {app}\LDView.exe; Flags: uninsdeletekey noerror
Root: HKCR; Subkey: Applications\LDView.exe; ValueType: string; ValueName: ; ValueData: LDView; Flags: uninsdeletekey
Root: HKCR; Subkey: Applications\LDView.exe\shell\open\command; ValueType: string; ValueName: ; ValueData: """{app}\LDView.exe"" ""%1"""; Tasks: registerfiles

Root: HKCR; Subkey: .ldr; ValueType: string; ValueName: ; ValueData: LDView.ldr; Flags: uninsdeletevalue; Tasks: registerfiles
Root: HKCR; Subkey: LDView.ldr; ValueType: string; ValueName: ; ValueData: LDraw Model; Flags: uninsdeletekey; Tasks: registerfiles
Root: HKCR; Subkey: LDView.ldr\DefaultIcon; ValueType: string; ValueName: ; ValueData: {app}\LDView.exe,3; Tasks: registerfiles
Root: HKCR; Subkey: LDView.ldr\shell\open\command; ValueType: string; ValueName: ; ValueData: """{app}\LDView.exe"" ""%1"""; Tasks: registerfiles

Root: HKCR; Subkey: .mpd; ValueType: string; ValueName: ; ValueData: LDView.mpd; Flags: uninsdeletevalue; Tasks: registerfiles
Root: HKCR; Subkey: LDView.mpd; ValueType: string; ValueName: ; ValueData: LDraw Multi-Part Dat; Flags: uninsdeletekey; Tasks: registerfiles
Root: HKCR; Subkey: LDView.mpd\DefaultIcon; ValueType: string; ValueName: ; ValueData: {app}\LDView.exe,4; Tasks: registerfiles
Root: HKCR; Subkey: LDView.mpd\shell\open\command; ValueType: string; ValueName: ; ValueData: """{app}\LDView.exe"" ""%1"""; Tasks: registerfiles
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Fast; ValueType: string; ValueName: _SessionPlaceholder; ValueData: DO NOT DELETE.; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Fast; ValueType: dword; ValueName: SortTransparent; ValueData: $00000000; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Fast; ValueType: dword; ValueName: DrawLightDats; ValueData: $00000000; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Fast; ValueType: dword; ValueName: PerformSmoothing; ValueData: $00000000; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Fast; ValueType: dword; ValueName: TextureStuds; ValueData: $00000000; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Fast with Edges; ValueType: string; ValueName: _SessionPlaceholder; ValueData: DO NOT DELETE.; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Fast with Edges; ValueType: dword; ValueName: SortTransparent; ValueData: $00000000; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Fast with Edges; ValueType: dword; ValueName: DrawLightDats; ValueData: $00000000; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Fast with Edges; ValueType: dword; ValueName: PerformSmoothing; ValueData: $00000000; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Fast with Edges; ValueType: dword; ValueName: TextureStuds; ValueData: $00000000; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Fast with Edges; ValueType: string; ValueName: FOV; ValueData: 20; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Fast with Edges; ValueType: dword; ValueName: Seams; ValueData: $00000000; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Fast with Edges; ValueType: dword; ValueName: ShowHighlightLines; ValueData: $00000001; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Fast with Edges; ValueType: dword; ValueName: ConditionalHighlights; ValueData: $00000000; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Fast with Edges; ValueType: dword; ValueName: SubduedLighting; ValueData: $00000001; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Fast with Edges; ValueType: dword; ValueName: UseSpecular; ValueData: $00000000; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Fast with Edges; ValueType: dword; ValueName: Lighting; ValueData: $00000000; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Quality; ValueType: string; ValueName: _SessionPlaceholder; ValueData: DO NOT DELETE.; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Quality; ValueType: dword; ValueName: LineSmoothing; ValueData: $00000001; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Quality; ValueType: dword; ValueName: UseQualityLighting; ValueData: $00000001; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Quality; ValueType: dword; ValueName: UseQualityStuds; ValueData: $00000001; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Quality with Edges; ValueType: string; ValueName: _SessionPlaceholder; ValueData: DO NOT DELETE.; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Quality with Edges; ValueType: dword; ValueName: LineSmoothing; ValueData: $00000001; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Quality with Edges; ValueType: dword; ValueName: UseQualityLighting; ValueData: $00000001; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Quality with Edges; ValueType: dword; ValueName: UseQualityStuds; ValueData: $00000001; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Quality with Edges; ValueType: string; ValueName: FOV; ValueData: 20; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Quality with Edges; ValueType: dword; ValueName: Seams; ValueData: $00000000; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Quality with Edges; ValueType: dword; ValueName: ShowHighlightLines; ValueData: $00000001; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Quality with Edges; ValueType: dword; ValueName: ConditionalHighlights; ValueData: $00000001; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Quality with Edges; ValueType: dword; ValueName: SubduedLighting; ValueData: $00000001; Flags: createvalueifdoesntexist; Tasks: prefsets
Root: HKCU; SubKey: Software\Travis Cobbs\LDView\Sessions\LDView Preset - Quality with Edges; ValueType: dword; ValueName: UseSpecular; ValueData: $00000000; Flags: createvalueifdoesntexist; Tasks: prefsets
