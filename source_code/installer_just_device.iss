#define AppVer "0.2.4.1"
#define AppName "Screen Capturer Recorder Device Only"

[UninstallRun]
Filename: regsvr32; WorkingDir: {app}; Parameters: /s /u PushDesktop.ax
[Run]
Filename: regsvr32; WorkingDir: {app}; Parameters: /s PushDesktop.ax
[Files]
Source: Release\PushDesktop.ax; DestDir: {app}
Source: ..\ChangeLog.txt; DestDir: {app}
Source: ..\README.TXT; DestDir: {app}; Flags: isreadme

[Setup]
AppName={#AppName}
AppVerName={#AppVer}
DefaultDirName={pf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayName={#AppName} uninstall
OutputBaseFilename=setup {#AppName} v{#AppVer}
OutputDir=..\releases

[Icons]
Name: {group}\Readme; Filename: {app}\README.TXT
Name: {group}\configure\Release Notes for v{#AppVer}; Filename: {app}\ChangeLog.txt
Name: {group}\configure\Uninstall; Filename: {uninstallexe}
Name: {group}\configure\re-register capture device after install msvcr100; Filename: regsvr32; WorkingDir: {app}; Parameters: PushDesktop.ax
