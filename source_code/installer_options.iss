#define AppVer "0.0.3"
#define AppName "Screen Capture Recorder Device"

[UninstallRun]
Filename: regsvr32; WorkingDir: {app}; Parameters: /s /u PushDesktop.ax
[Run]
Filename: regsvr32; WorkingDir: {app}; Parameters: /s PushDesktop.ax
[Files]
Source: Release\PushDesktop.ax; DestDir: {app}
Source: ..\README.TXT; DestDir: {app}; Flags: isreadme
[Setup]
AppName={#AppName}
AppVerName={#AppVer}
DefaultDirName={pf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayName={#AppName} uninstall
OutputBaseFilename=setup {#AppName} v{#AppVer}
[Icons]
Name: {group}\Uninstall it; Filename: {uninstallexe}
