This program allows one to "record" your desktop, or even stream it, with the help of VLC.

It also includes some helper "record your screen capture" and setup utilities.
  In order to use those you'll want/need the java JRE installed on your system first.

It also includes a free, general purpose, open source directshow desktop/screen source capture filter.
You can use this with any (directshow compatible) program to capture or stream, like VLC.

It also includes a directshow source capture filter device for recording "what you hear" in windows 7/vista.
  see https://github.com/rdp/virtual-audio-capture-grabber-device
  and some helper utilities for both.

== Installation ==

Download installer and run, from 

  https://sourceforge.net/projects/screencapturer/files
    (sorry the files  sourceforge: github limits their disk usage space and I'm hitting up against the limit)

 NB that if you install the 64 bit device, you'll need to install the MSVC 2010 64 bit runtime first : http://www.microsoft.com/en-us/download/details.aspx?id=14632

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


== Streaming ==

You can stream using it with ffmpeg (to an rtmp server) or to UDP, or use VLC (itself as a server, or VLC to an rtmp server) or
use Flash Media Live Encoder, to some flash server.

"I can confirm also that VLC is very nice for everything but real-time
steaming video, since it has a 1-second or more video delay" (use ffmpeg instead?)

ffplay to say it nicely: sucks big time.  The image quality is horrible, pixelated.

VLC can receive a UDP stream and look good: 
vnc -vvv udp://127.0.0.1:1234

== Configuration ==

By default, it captures the "full screen" of the main desktop monitor (all windows, overlapping, there).

To configure it differently, run the provided "configuration setup utilities" or 
adjust registry settings before starting a run (advanced users only):

HKEY_CURRENT_USER\Software\os_screen_capture

with DWORD keys respected of start_x ... (see the file configuration_setup_utility\setup_screen_tracker_params.rb for a full list)
  
ex: see incoming.reg (though NB that those values are in hex, so editing that file is a bit tedious--
I just use regedit or the accompanying script utilities).  

To "reset" a value delete its key.

== Trouble shooting/Feedback/Questions ==

it's too slow!
  Run the "benchmark your capture speed" utility to see how slow your system can capture.
  A few things that can help: if you're on vista+
    turn off aero display manager (esp. if you have dual monitors, this can help).
    http://www.howtogeek.com/howto/windows-vista/disable-aero-on-windows-vista
	It captures much more quickly if aero is turned off (as 
    in with aero, capturing a 650x976 window takes 50ms, without aero, 3ms.
	The rest of a single screenshot capture takes about 7ms (sum 
    10ms without aero), so you can see the relative cost it adds [!]
	There's also a registry setting to have it turned off "automatically"
	I have only been able to get max 15 fps when capturing aero [ping me if you want me to look into improving this somewhat, I have some ideas...]
  disabling or enabling "hardware acceleration" for the desktop might help, too, or setting it to one stop above none
  try switching from 32 to 16 bit output (the filter), and/or your desktop depth itself--or both! fastest is 16 bit display with capture in 16 bit mode--400 fps whoa!).  

Note that if your output is, say, going to be 10 fps "actually used" in the end, ex:
$ ffmpeg -f dshow -i video=video-capture-recorder yo.mp4 -r 10 # output file is only 10 fps.

Then to save cpu, a "good" application will also instruct the "source fps" to be 10 fps
that they pass on to this filter, and everybody's in 10 fps and hopefully happy, and cpu is saved.
ex: vlc.exe dshow:// ... :dshow-fps=10
vlc then passes this in to SetFormat after negotiation, and the device accepts it, and feeds you at 10 fps.

Some "bad" directshow applications don't pass this in.
So there may be a case where it is wasting cpu somehow, by capturing too many or the like.
So for those programs, you'll want to set the force_max_fps parameter, which will limit its
capture frequency.

Setting force_max_fps to greater than 30 also allows you to get fps greater than 30.  It "enables" them,
by giving it a higher max default.  I didn't think people would normally care/want them so there you have.

Other feedback/problems/questions ping me roger-projects@googlegroups.com or browse the mailing list: http://groups.google.com/group/roger-projects

==  Redistribution ==

You have to register the devicemanually (regsvr32 PushDesktop.ax, may require administrator rights) as part of your install.
Also note that end users also need the MSVC 2010 redistributable previously installed (or you have to make it available 
in the same dir as your exe) for the dll to work/install, in case end users don't have it installed, in that case).

== Code/Future work/Attributions ==

The code is gently based on/lifted from ("inspired by")
  ....\Microsoft SDKs\Windows\v7.1\Samples\multimedia\directshow\filters\pushsource as well as some other dshow demos around the 'net.
  so you'll probably want to install the Windows SDK before messing around with the source code.

I can add features upon reasonable demand.

Basically you want a feature, ping me, you got it.

To build it locally, install VS Express 2010, install Microsoft SDK, open up your equivalent of 
\Program Files\Microsoft SDKs\Windows\v7.1\Samples\multimedia\directshow\baseclasses\baseclasses.sln
build it (debug)
now add to your local build (project properties, from the source_code\pushdesktop.sln) adjust the VC++ Directories (Include/Library) 
to point to your various baseclass directories.

== More Attributions ==

The included speed test utility was originally from http://stereopsis.com/blttest
(bitblt is quite hardware dependent, so I'm told, so it's good to be able to test its speed).

FFMpeg binaries are also included, see their site for licensing details/source: http://ffmpeg.org