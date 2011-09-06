#define AppVer "0.0.4"
#define AppName "Screen Capture Recorder Device"

[UninstallRun]
Filename: regsvr32; WorkingDir: {app}; Parameters: /s /u PushDesktop.ax
[Run]
Filename: regsvr32; WorkingDir: {app}; Parameters: /s PushDesktop.ax
[Files]
Source: Release\PushDesktop.ax; DestDir: {app}
Source: ..\README.TXT; DestDir: {app}; Flags: isreadme
Source: ..\configuration_setup_utility\*.*; DestDir: {app}\configuration_setup_utility; Flags: recursesubdirs
Source: ..\troubleshooting_benchmarker\BltTest\Release\BltTest.exe; DestDir: {app}

[Setup]
AppName={#AppName}
AppVerName={#AppVer}
DefaultDirName={pf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayName={#AppName} uninstall
OutputBaseFilename=setup {#AppName} v{#AppVer}
[Icons]
Name: {group}\Uninstall; Filename: {uninstallexe}
Name: {group}\configuration_setup_utility; Filename: {app}\configuration_setup_utility\edit_config.bat; WorkingDir: {app}\configuration_setup_utility
Name: {group}\benchmark your screen capture speeds; Filename: {app}\BltTest.exe; WorkingDir: {app}
Name: {group}\record using current settings; Filename: java; WorkingDir: {app}\configuration_setup_utility; Parameters: -jar jruby-complete-1.6.4.jar start_stop_recording_just_desktop.rb
