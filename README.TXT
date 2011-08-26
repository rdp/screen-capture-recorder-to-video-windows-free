This device allows you to "record" your desktop, or stream it.
Basically this aims to be a free, general purpose, 
open source directshow desktop/screen source capture filter device.
Use it with your (directshow compatible) program to capture or stream.
I couldn't find any open source equivalent versions so made my own :)

== Installation ==

Download installer from 

  https://github.com/rdp/screen-capture-recorder-program/downloads

== Usage ==

Anything that can read from Directshow Capture Devices (ex: VLC) should be able to use it as input.

VLC example: http://betterlogic.com/roger/2010/07/how-to-use-vlc-as-a-free-open-source-alternative-to-playon-tv

ffmpeg also can use it:

$ ffplay -f dshow -i audio="virtual-audio-capturer":video="screen-capture-recorder"

(in this example, assuming
you also had installed the audio device from https://github.com/rdp/virtual-audio-capture-grabber-device
which probably isn't necessary for < vista.)

Let me know if you want an easier way made for just capturing the screen etc.
Another program that leverages it internally is sensible cinema 
(avisynth also works with it).

== Configuration ==

By default, it captures the "full screen" of the main desktop monitor.

To configure it differently, adjust these registry settings before starting a run with it:

HKEY_CURRENT_USER\Software\os_screen_capture
with DWORD keys respected of:
  start_x, start_y, width, height, fps
  # unimplemented, but will be if there is ever any request for them:
  # monitor_number, hwnd_to_use, show_mouse
  
ex: see incoming.reg (though NB that those values are in hex, so editing that file is a bit tedious).  

The jruby setup script provided in configuration_setup_utility is another way to set them up.

If any value registry value is set to 0, that means "not set" so it uses the default for that value.

== Trouble shooting ==

it's too slow!
  turn off aero display manager (esp. if you have dual monitors, this may help).
  disabling/enabling "hardware acceleration" for your desktop might help, too.
  try switching from 16 to 32 bit 
Run the troubleshooting_benchmarker\BltTest\Release\BltTest.exe file.
Run it to benchmark your system and compare the various settings.
  What you care about is the "reverse" timing speed in its output display.

Other things ping me rogerdpack@gmail.com or mailing list: http://groups.google.com/group/roger-projects

== Future work ==

It is gently lifted from ("inspired by")
  ....\Microsoft SDKs\Windows\v7.1\Samples\multimedia\directshow\filters\pushsource as well as some other dshow demos around.
  so you'll probably want to install the Windows SDK before messing around with the source code.

Message me if interested in it, or with any feature requests :)

I can add features upon reasonable demand.

Basically you want a feature, ping me, you got it.

the included blttest utility is originally from http://stereopsis.com/blttest
(blttest is quite hardware dependent, or so I'm told).