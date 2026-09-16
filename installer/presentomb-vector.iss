#define AppName "presentomb vector"
#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#define Publisher "presentomb"
#ifndef BuildRoot
  #define BuildRoot "..\build-gon\PresentombDSO2_artefacts\Release"
#endif
#define Vst3Name "presentomb vector.vst3"
#define StandaloneExe "presentomb vector.exe"

[Setup]
AppId={{7A70A247-6B05-4939-B0DB-DF7F1AF3E16E}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#Publisher}
DefaultDirName={autopf}\presentomb\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=presentomb-vector-{#AppVersion}-windows
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
UninstallDisplayName={#AppName}

[Types]
Name: "plugin"; Description: "VST3 plugin only"
Name: "standalone"; Description: "Standalone app only"
Name: "full"; Description: "VST3 plugin and standalone app"
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plugin"; Types: plugin full custom
Name: "standalone"; Description: "Standalone app"; Types: standalone full custom

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut for the standalone app"; Components: standalone; Flags: unchecked

[Files]
Source: "{#BuildRoot}\VST3\{#Vst3Name}\*"; DestDir: "{commoncf64}\VST3\{#Vst3Name}"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildRoot}\Standalone\{#StandaloneExe}"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#StandaloneExe}"; Components: standalone
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#StandaloneExe}"; Components: standalone; Tasks: desktopicon

[Run]
Filename: "{app}\{#StandaloneExe}"; Description: "Launch {#AppName}"; Components: standalone; Flags: nowait postinstall skipifsilent unchecked
