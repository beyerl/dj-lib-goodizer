; Inno Setup script for DJ Library Goodizer (Windows installer).
; Prerequisite: run windeployqt into dist\dj-lib-goodizer first so the Qt
; runtime sits beside the executable, then: iscc packaging\windows\installer.iss

#define AppName "DJ Library Goodizer"
#define AppVersion "0.9.0"
#define AppExe "dj-lib-goodizer.exe"

[Setup]
AppName={#AppName}
AppVersion={#AppVersion}
DefaultDirName={autopf}\DJ Library Goodizer
DefaultGroupName=DJ Library Goodizer
UninstallDisplayIcon={app}\{#AppExe}
OutputBaseFilename=dj-lib-goodizer-setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Files]
; Staged portable build produced by windeployqt.
Source: "..\..\dist\dj-lib-goodizer\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs

[Icons]
Name: "{group}\DJ Library Goodizer"; Filename: "{app}\{#AppExe}"
Name: "{commondesktop}\DJ Library Goodizer"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"

[Run]
Filename: "{app}\{#AppExe}"; Description: "Launch DJ Library Goodizer"; Flags: nowait postinstall skipifsilent
