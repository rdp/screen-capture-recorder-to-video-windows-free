#define AppVer "0.12.5"

#define AppName "Screen Capturer Recorder"
; AppId === AppName by default BTW

; To use this, run it from within virtual ...

[Run]
Filename: {app}\vendor\vcredist_x86.exe; Parameters: "/passive /Q:a /c:""msiexec /qb /i vcredist.msi"" "; StatusMsg: Installing MSVC 2010 RunTime...
Filename: {app}\vendor\vcredist_x64.exe; Parameters: "/passive /Q:a /c:""msiexec /qb /i vcredist.msi"" "; StatusMsg: Installing MSVC 2010 64 bit RunTime...; MinVersion: 0,6.0.6000; Check: IsWin64
Filename: regsvr32; WorkingDir: {app}; Parameters: /s screen-capture-recorder.dll
Filename: regsvr32; WorkingDir: {app}; Parameters: /s screen-capture-recorder-x64.dll; Check: IsWin64
Filename: regsvr32; WorkingDir: {app}; Parameters: /s vendor\virtual-audio\audio_sniffer.dll; MinVersion: 0,6.0.6000
Filename: regsvr32; WorkingDir: {app}; Parameters: /s vendor\virtual-audio\audio_sniffer-x64.dll; MinVersion: 0,6.0.6000; Check: IsWin64

; TODO clear registry, prefs on uninstall?

[UninstallRun]
Filename: regsvr32; WorkingDir: {app}; Parameters: /s /u screen-capture-recorder.dll
Filename: regsvr32; WorkingDir: {app}; Parameters: /s /u screen-capture-recorder-x64.dll; Check: IsWin64
Filename: regsvr32; WorkingDir: {app}; Parameters: /s /u vendor\virtual-audio\audio_sniffer.dll; MinVersion: 0,6.0.6000
Filename: regsvr32; WorkingDir: {app}; Parameters: /s /u vendor\virtual-audio\audio_sniffer-x64.dll; MinVersion: 0,6.0.6000; Check: IsWin64

[Files]
Source: source_code\Win32\Release\screen-capture-recorder.dll; DestDir: {app}
Source: source_code\x64\releasex64\screen-capture-recorder-x64.dll; DestDir: {app}
Source: README.TXT; DestDir: {app}; Flags: isreadme
Source: ChangeLog.txt; DestDir: {app}
; includes vendor/ffmpeg et al
Source: configuration_setup_utility\*.*; DestDir: {app}\configuration_setup_utility; Flags: recursesubdirs
Source: vendor\troubleshooting_benchmarker\BltTest\Release\BltTest.exe; DestDir: {app}
Source: vendor\vcredist_*.exe; DestDir: {app}\vendor
; ruby scripts read version from this
Source: innosetup_installer_options.iss; DestDir: {app}\

; include latest dll's from virtual audio capturer...
Source: ..\source_code\Release\audio_sniffer.dll; DestDir: {app}\vendor\virtual-audio;
Source: ..\source_code\x64\Release\audio_sniffer-x64.dll; DestDir: {app}\vendor\virtual-audio;

[Setup]
AppName={#AppName}
AppVerName={#AppVer}
DefaultDirName={pf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayName={#AppName} uninstall
OutputBaseFilename=Setup {#AppName} v{#AppVer}
OutputDir=releases

; remove previous versions' outdated icons [lame innosetup, lame]
[InstallDelete]
Type: filesandordirs; Name: {group}\*;

[Icons]
Name: {group}\configure\Release Notes; Filename: {app}\ChangeLog.txt
Name: {group}\configure\Readme; Filename: {app}\README.TXT
Name: {group}\configure\configure by setting specific screen capture numbers; Filename: {app}\configuration_setup_utility\generic_run_rb.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: setup_via_numbers.rb; Flags: runminimized
Name: {group}\configure\benchmark your machines screen capture speed; Filename: {app}\BltTest.exe; WorkingDir: {app}
Name: {group}\configure\configure by resizing a transparent window; Filename: {app}\configuration_setup_utility\generic_run_rb.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: window_resize.rb; Flags: runminimized
Name: {group}\configure\Display current capture settings; Filename: {app}\configuration_setup_utility\generic_run_rb.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: setup_via_numbers.rb --just-display-current-settings
Name: {group}\configure\Uninstall {#AppName}; Filename: {uninstallexe}
Name: {group}\Record\Record or stream video and or audio; Filename: {app}\configuration_setup_utility\generic_run_rb.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: record_with_buttons.rb; Flags: runminimized
Name: {group}\Record\Record audio by clicking a button; Filename: {app}\configuration_setup_utility\generic_run_rb.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: record_with_buttons.rb --just-audio-default; Flags: runminimized
Name: {group}\record\broadcast\setup local audio broadcast streaming server; Filename: {app}\configuration_setup_utility\generic_run_rb.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: broadcast_server_setup.rb; Flags: runminimized
Name: {group}\record\broadcast\restart local audio streaming server with same setup as was run previous; Filename: {app}\configuration_setup_utility\generic_run_rb.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: broadcast_server_setup.rb --redo-with-last-run; Flags: runminimized
Name: {group}\record\broadcast\stream desktop local LAN; Filename: {app}\configuration_setup_utility\generic_run_rb.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: stream_desktop_p2p.rb; Flags: runminimized;

[Languages]
Name: fr; MessagesFile: compiler:Languages\French.isl; 
Name: de; MessagesFile: compiler:Languages\German.isl; 
Name: "en"; MessagesFile: "compiler:Default.isl"
