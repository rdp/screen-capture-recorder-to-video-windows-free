Basically this aims to be a free, general purpose, open source directshow desktop screen source capture filter.
I.e. it allows you to "record" your desktop into your (directshow compatible) program or what not.
I couldn't find any open source versions so made my own :)

It is gently lifted from ("inspired by")
  ....\Microsoft SDKs\Windows\v7.1\Samples\multimedia\directshow\filters\pushsource as well as some other dshow demos around.
  so you'll probably want to install the Windows SDK before messing around with the source code.

Message me if interested in it, or with any feature requests :)

== Installation ==

Download from https://github.com/rdp/open-source-directshow-screen-desktop-capture-source-filter/downloads

== Usage ==

Anything that can read from Directshow Capture Devices (ex: VLC) should be able to use it.

Let me know if you want an easier way of capturing the screen.

Another example thing that uses it is sensible cinema (uses it via ffmpeg/avisynth).

== Configuration ==

By default, it captures the "full screen" of the main desktop monitor.

To configure it differently, adjust these registry settings before starting:

HKEY_CURRENT_USER\Software\os_screen_capture
with DWORD keys respected of:
  start_x, start_y, width, height
  # unimplemented, but will be if there is ever any request for them:
  # monitor_number, fps, hwnd_to_use, show_mouse
  
ex: see incoming.reg (though NB that those values are in hex, so editing that file is a bit tedious).  

The jruby setup script in configuration_setup_utility is another way to set them up.
If any value registry value is set to 0, that means "not set" so it uses the default for that value.

== trouble shooting ==

it's too slow:
  turn off aero display manager (esp. if you have dual monitors, this may help).
  disabling/enabling "hardware acceleration" for your desktop might help, too.
  try switching from 16 to 32 bit 
Within BltTest there is  troubleshooting_benchmarker\BltTest\Release\BltTest.exe file.
Run it to benchmark your system and compare the various settings.
  What you care about is the "reverse" timing speed in its output display.

Other things ping me rogerdpack@gmail.com or mailing list: http://groups.google.com/group/roger-projects

== Future work ==

I can add features upon demand.

Basically you want a feature, ping me, you got it.

blttest is originally from http://stereopsis.com/blttest
(which is quite hardware dependent, or so I'm told).