#define AppVer "0.3.8"
#define AppName "Screen Capturer Recorder"
; AppId === AppName by default BTW

[UninstallRun]
Filename: regsvr32; WorkingDir: {app}; Parameters: /s /u PushDesktop.ax
Filename: regsvr32; WorkingDir: {app}; Parameters: /s /u vendor\audio_sniffer.026.ax
[Run]
Filename: regsvr32; WorkingDir: {app}; Parameters: /s PushDesktop.ax
Filename: regsvr32; WorkingDir: {app}; Parameters: /s vendor\audio_sniffer.026.ax
[Files]
Source: source_code\Win32\Release\PushDesktop.ax; DestDir: {app}
Source: README.TXT; DestDir: {app}; Flags: isreadme
Source: ChangeLog.txt; DestDir: {app}
Source: configuration_setup_utility\*.*; DestDir: {app}\configuration_setup_utility; Flags: recursesubdirs
Source: vendor\troubleshooting_benchmarker\BltTest\Release\BltTest.exe; DestDir: {app}
Source: vendor\*.*; DestDir: {app}\vendor; Flags: recursesubdirs; MinVersion: 0,6.0.6000

[Setup]
AppName={#AppName}
AppVerName={#AppVer}
DefaultDirName={pf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayName={#AppName} uninstall
OutputBaseFilename=setup {#AppName} v{#AppVer}
OutputDir=releases

[Icons]
Name: {group}\Record\record screen or audio using current settings for a variable number of seconds; Filename: {app}\configuration_setup_utility\timed_recording.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: 
Name: {group}\Readme; Filename: {app}\README.TXT
Name: {group}\configure\configure by setting specific screen capture numbers; Filename: {app}\configuration_setup_utility\edit_config.bat; WorkingDir: {app}\configuration_setup_utility
Name: {group}\configure\benchmark your machines screen capture speed; Filename: {app}\BltTest.exe; WorkingDir: {app}
Name: {group}\configure\configure screen capture by resizing a transparent window; Filename: {app}\configuration_setup_utility\resizable_window.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: 
Name: {group}\configure\Release Notes for v{#AppVer}; Filename: {app}\ChangeLog.txt
Name: {group}\Uninstall {#AppName}; Filename: {uninstallexe}
Name: {group}\configure\re-register capture device after install msvcr100, if necessary; Filename: regsvr32; WorkingDir: {app}; Parameters: PushDesktop.ax
Name: {group}\configure\Display current capture settings; Filename: {app}\configuration_setup_utility\display_current_settings.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: 
Name: {group}\Broadcast\setup local audio streaming server; Filename: {app}\configuration_setup_utility\run_broadcast_audio_server.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: 
Name: {group}\Broadcast\restart local audio streaming server with same setup as was run previous; Filename: {app}\configuration_setup_utility\rerun_audio_server.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: 
