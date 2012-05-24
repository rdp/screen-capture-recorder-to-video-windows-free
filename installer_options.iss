#define AppVer "0.6.7"

#define AppName "Screen Capturer Recorder 32-bit"
; AppId === AppName by default BTW

[Run]
Filename: {app}\vendor\vcredist_x86.exe; Parameters: "/passive /Q:a /c:""msiexec /qb /i vcredist.msi"" "; StatusMsg: Installing 2010 RunTime...
Filename: regsvr32; WorkingDir: {app}; Parameters: /s screen-capture-recorder.dll
Filename: regsvr32; WorkingDir: {app}; Parameters: /s vendor\audio_sniffer.030.ax; MinVersion: 0,6.0.6000

[UninstallRun]
Filename: regsvr32; WorkingDir: {app}; Parameters: /s /u screen-capture-recorder.dll
Filename: regsvr32; WorkingDir: {app}; Parameters: /s /u vendor\audio_sniffer.030.ax; MinVersion: 0,6.0.6000

[Files]
Source: source_code\Win32\Release\screen-capture-recorder.dll; DestDir: {app}
Source: README.TXT; DestDir: {app}; Flags: isreadme
Source: ChangeLog.txt; DestDir: {app}
Source: configuration_setup_utility\*.*; DestDir: {app}\configuration_setup_utility; Flags: recursesubdirs
Source: vendor\troubleshooting_benchmarker\BltTest\Release\BltTest.exe; DestDir: {app}
Source: vendor\*.ax; DestDir: {app}\vendor
;Source: vendor\*.dll; DestDir: {app}\vendor
Source: vendor\vcredist_x86.exe; DestDir: {app}\vendor

[Setup]
AppName={#AppName}
AppVerName={#AppVer}
DefaultDirName={pf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayName={#AppName} uninstall
OutputBaseFilename=Setup {#AppName} v{#AppVer}
OutputDir=releases

[Icons]
Name: {group}\Readme; Filename: {app}\README.TXT
Name: {group}\configure\Release Notes; Filename: {app}\ChangeLog.txt
Name: {group}\Record\record screen and or audio using current settings for a variable number of seconds; Filename: {app}\configuration_setup_utility\generic_run_rb.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: timed_recording.rb; Flags: runminimized
Name: {group}\Record\Record by clicking a start button; Filename: {app}\configuration_setup_utility\generic_run_rb.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: record_with_buttons.rb; Flags: runminimized
Name: {group}\configure\configure by setting specific screen capture numbers; Filename: {app}\configuration_setup_utility\generic_run_rb.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: setup_via_numbers.rb; Flags: runminimized
Name: {group}\configure\benchmark your machines screen capture speed; Filename: {app}\BltTest.exe; WorkingDir: {app}
Name: {group}\configure\configure by resizing a transparent window; Filename: {app}\configuration_setup_utility\generic_run_rb.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: window_resize.rb; Flags: runminimized
Name: {group}\configure\Display current capture settings; Filename: {app}\configuration_setup_utility\generic_run_rb.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: setup_via_numbers.rb --just-display-current-settings
Name: {group}\Broadcast audio\setup local audio streaming server; Filename: {app}\configuration_setup_utility\generic_run_rb.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: broadcast_server_setup.rb; Flags: runminimized
Name: {group}\Broadcast audio\restart local audio streaming server with same setup as was run previous; Filename: {app}\configuration_setup_utility\generic_run_rb.bat; WorkingDir: {app}\configuration_setup_utility; Parameters: broadcast_server_setup.rb --redo-with-last-run; Flags: runminimized
Name: {group}\configure\Uninstall {#AppName}; Filename: {uninstallexe}
