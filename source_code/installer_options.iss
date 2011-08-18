#define AppVer "0.0.1"
#define AppName "pushdesktop"

[UninstallRun]
Filename: regsvr32; WorkingDir: {app}; Parameters: /u PushDesktop.ax
[Run]
Filename: regsvr32; WorkingDir: {app}; Parameters: PushDesktop.ax
[Files]
Source: C:\dev\ruby\pushdesktop\source_code\Release\PushDesktop.ax; DestDir: {app}
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
