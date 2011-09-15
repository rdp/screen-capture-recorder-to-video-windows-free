This program allows one to "record" your desktop, or even stream it, with the help of VLC.

It also includes some helper "record your screen capture" and setup utilities.

It also includes a free, general purpose, open source directshow desktop/screen source capture filter.
You can use this with any (directshow compatible) program to capture or stream, like VLC.

== Installation ==

Download installer and run, from 

 https://sourceforge.net/projects/screencapturer/files

== Usage ==

Use the helper programs within Programs -> Screen Capture Recorder

Or use anything that can read from a Directshow Capture Device (ex: VLC).

a VLC example: http://betterlogic.com/roger/2010/07/how-to-use-vlc-as-a-free-open-source-alternative-to-playon-tv

an ffmpeg example:

$ ffplay -f dshow -i video="screen-capture-recorder"

or

$ ffmpeg -f dshow  -i video="screen-capture-recorder"  -r 20 -t 10 screen-capture.mp4 # -t 10 for 10 seconds recording

or combine it with recorded audio (with ffmpeg) if desired:

$ ffplay -f dshow -i audio="virtual-audio-capturer":video="screen-capture-recorder" # this example gives feedback though...

(assuming you also had also installed the audio device from 
https://github.com/rdp/virtual-audio-capture-grabber-device
which probably isn't useful/needed for < vista, so your audio="..." setting would change.)

Another program that leverages it internally is sensible cinema's upconversion (pins it to the location of a given HWND window even).

Avisynth also works with it.  Add it to a filter graph using graphedit, then use DirectShowSource as your input source.

Let me know if you want an easier way made for anything.

== Configuration ==

By default, it captures the "full screen" of the main desktop monitor (all windows, overlapping, there).

To configure it differently, run the provided "configuration setup utilities" or 
adjust registry settings before starting a run (advanced users only):

HKEY_CURRENT_USER\Software\os_screen_capture
with DWORD keys respected of:
  start_x, start_y, width, height, fps
  # monitor_number, hwnd_to_use, show_mouse   # unimplemented, but can be created if there is any request for them:
  
ex: see incoming.reg (though NB that those values are in hex, so editing that file is a bit tedious).  

If any value registry value is set to 0, that means "not set" so it uses the default for that value.

== Trouble shooting/Feedback/Questions ==

it's too slow!
  turn off aero display manager (esp. if you have dual monitors, this can help).
    http://www.howtogeek.com/howto/windows-vista/disable-aero-on-windows-vista/
  disabling/enabling "hardware acceleration" for your desktop might help, too.
  try switching from 16 to 32 bit 
  Run the "benchmark your capture speed" utility to see how it affects it.

Other feedback/problems/questions ping me rogerdpack@gmail.com or mailing list: http://groups.google.com/group/roger-projects

== Code/Future work ==

The code is gently lifted from ("inspired by")
  ....\Microsoft SDKs\Windows\v7.1\Samples\multimedia\directshow\filters\pushsource as well as some other dshow demos around.
  so you'll probably want to install the Windows SDK before messing around with the source code.

I can add features upon reasonable demand.

Basically you want a feature, ping me, you got it.

the included speed test utility was originally from http://stereopsis.com/blttest
(bitblt is quite hardware dependent, or so I'm told).

FFMpeg also included, see their site for licensing details/source: ffmpeg.org