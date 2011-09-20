#define AppVer "0.2.4.1"
#define AppName "Screen Capturer Recorder"

[UninstallRun]
Filename: regsvr32; WorkingDir: {app}; Parameters: /s /u PushDesktop.ax
[Run]
Filename: regsvr32; WorkingDir: {app}; Parameters: /s PushDesktop.ax
[Files]
Source: Release\PushDesktop.ax; DestDir: {app}
Source: ..\README.TXT; DestDir: {app}; Flags: isreadme
Source: ..\ChangeLog.txt; DestDir: {app}
Source: ..\configuration_setup_utility\*.*; DestDir: {app}\configuration_setup_utility; Flags: recursesubdirs
Source: ..\troubleshooting_benchmarker\BltTest\Release\BltTest.exe; DestDir: {app}

[Setup]
AppName={#AppName}
AppVerName={#AppVer}
DefaultDirName={pf}\{#AppName}
DefaultGroupName={#AppName}
UninstallDisplayName={#AppName} uninstall
OutputBaseFilename=setup {#AppName} v{#AppVer}
OutputDir=..\releases

[Icons]
Name: {group}\record screen using current settings for a specific number of seconds; Filename: java; WorkingDir: {app}\configuration_setup_utility; Parameters: -jar jruby-complete-1.6.4.jar timed_recording.rb
Name: {group}\Readme; Filename: {app}\README.TXT
Name: {group}\configure\configure by setting specific numbers; Filename: {app}\configuration_setup_utility\edit_config.bat; WorkingDir: {app}\configuration_setup_utility
Name: {group}\configure\benchmark your machines screen capture speed; Filename: {app}\BltTest.exe; WorkingDir: {app}
Name: {group}\configure\configure by resizing a transparent window; Filename: javaw; WorkingDir: {app}\configuration_setup_utility; Parameters: -jar jruby-complete-1.6.4.jar window_resize.rb
Name: {group}\configure\Release Notes for v{#AppVer}; Filename: {app}\ChangeLog.txt
Name: {group}\configure\Uninstall; Filename: {uninstallexe}
Name: {group}\configure\re-register capture device after install msvcr100; Filename: regsvr32; WorkingDir: {app}; Parameters: PushDesktop.ax
Name: {group}\configure\Display current capture settings; Filename: java; WorkingDir: {app}\configuration_setup_utility; Parameters: -jar jruby-complete-1.6.4.jar setup_screen_tracker_params.rb --just-display-current-settings
