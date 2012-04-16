This program allows one to "record" your desktop, or even stream it, with the help of VLC.

It also includes some helper "record your screen capture" and setup utilities.
  In order to use those you'll want/need the java JRE installed on your system first.

It also includes a free, general purpose, open source directshow desktop/screen source capture filter.
You can use this with any (directshow compatible) program to capture or stream, like VLC.

It also includes a directshow source capture filter device for recording "what you hear" in windows 7/vista.
  see https://github.com/rdp/virtual-audio-capture-grabber-device
  and some utilities for it.

Note that turning off the aero windows manager (if you're on vista) can dramatically 
increase speed of capture.
"Another trick I have found to be necessary is to go into the Display Properties->Settings->Advanced->Troubleshoot and slide the Hardware Acceleration slider to one stop above None."

== Installation ==

Download installer and run, from 

  https://sourceforge.net/projects/screencapturer/files
    (sorry it's on sourceforge: github limits their disk usage space and I'm hitting up against the limit)

 NB that you may need need to install the MSVC 2010 runtime distro first, if it's not already
 installed on your machine (typically, it already is).
 http://www.microsoft.com/download/en/details.aspx?id=5555

== Usage ==

Use some helper programs provided in 
 Start menu -> Programs -> Screen Capture Recorder -> XXX

Or use any 3rd party program that can read from a Directshow Capture Device (ex: VLC).

VLC example: http://betterlogic.com/roger/2010/07/how-to-use-vlc-as-a-free-open-source-alternative-to-playon-tv

ffmpeg example (requires ffmpeg 32 bit -- 64 bit is available upon request):

  $ ffplay -f dshow -i video="screen-capture-recorder"

or

  $ ffmpeg -f dshow  -i video="screen-capture-recorder"  -r 20 -t 10 screen-capture.mp4 # -t 10 for 10 seconds recording

or combine it with recording "what you hear" audio [vista/windows 7] (using ffmpeg in this example):

  $ ffplay -f dshow -i audio="virtual-audio-capturer":video="screen-capture-recorder" # this example gives feedback be careful...
  
The audio device name can be other dshow audio devices, as well.

Avisynth also works with it.  Add it to a filter graph using graphedit, then use DirectShowSource as your 
input source with that graphedit filename specified.

gstreamer works, as well:

  $ gst-launch.exe dshowvideosrc device-name=screen-capture-recorder ! ffmpegcolorspace ! directdrawsink

I've even had Skype accidentally use it, thinking it was my webcam.
Let me know if you want an easier way made for it for anything.

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
  Run the "benchmark your capture speed" utility to see how slow it is.
  A few things that can help: if you're on vista+
    turn off aero display manager (esp. if you have dual monitors, this can help).
    http://www.howtogeek.com/howto/windows-vista/disable-aero-on-windows-vista/
  disabling or enabling "hardware acceleration" for the desktop might help, too.
  try switching from 32 to 16 bit.  It's a tidge faster to capture (90fps compared to 80fps on my box)
  It captures much more quickly if aero is turned off (as 
  in with aero, capturing a 650x976 window takes 50ms, without aero, 
  3ms).  The rest of a single screenshot capture takes about 7ms (sum 
  10ms without aero), so you can see the relative cost it adds [!]
  If you have dual core, then theoretically capturing with aero on will 
  just cause the capture thread to probably eat "an entire core" if you are trying to capture at 
  20 fps or higher.  So it may be worth it, if you have the extra cpu, to not care.

Note that if your output is, say, going to be 10 fps "used" in the end.
Ex: ffmpeg -f dshow -i video=video-capture-recorder yo.mp4 -r 10 # output file is only 10 fps

Then to save cpu, the "good" directshow applications have their own "source fps" parameter,
that they pass on to this filter.
ex: vlc.exe dshow:// ... :dshow-fps=1.5
vlc then passes this in to SetFormat after negotiation, and the device accepts it.

Some "bad" directshow applications don't pass this in.
So there may be a case where it is wasting cpu somehow, by capturing too many or the like.
So for those programs, you'll want to set the force_max_fps parameter, which will limit its
capture frequency.  It may be the only way to pull in some rogue applications (ffmpeg <cough>).

Setting force_max_fps to greater than 30 also allows you to get fps greater than 30.  It "enables" them,
by giving it a higher max default.  I didn't think people would normally care/want them so there you have.

Other feedback/problems/questions ping me rogerdpack@gmail.com or mailing list: http://groups.google.com/group/roger-projects

==  Redistribution ==

You can use this/redistribute it with your own software if you wish by just redistributing the DLL 
(PushDesktop.ax) alongside with your app (see installer_just_device.iss).
You'd have to register it manually (regsvr32 PushDesktop.ax, may need administrator rights) as part of your install.
Also note that end users also need the MSVC 2010 redistributable previously installed (or you have to make it available 
in the same dir as your exe) for the dll to work/install, in case end users don't have it installed, in that case).
Or you may be able to recompile it to work around that, etc.

== Code/Future work/Attributions ==

The code is gently lifted from ("inspired by")
  ....\Microsoft SDKs\Windows\v7.1\Samples\multimedia\directshow\filters\pushsource as well as some other dshow demos around.
  so you'll probably want to install the Windows SDK before messing around with the source code.

I can add features upon reasonable demand.

Basically you want a feature, ping me, you got it.

To build it locally, install VS Express 2010, install Microsoft SDK, open up your equivalent of 
\Program Files\Microsoft SDKs\Windows\v7.1\Samples\multimedia\directshow\baseclasses\baseclasses.sln
build it (debug)
now add to your local build (project properties, from the source_code\pushdesktop.sln) adjust the VC++ Directories (Include/Library) 
to point to your baseclasses.

== Attributions ==

the included speed test utility was originally from http://stereopsis.com/blttest
(bitblt is quite hardware dependent, or so I'm told).

FFMpeg also included, see their site for licensing details/source: ffmpeg.org