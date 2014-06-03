This program allows one to "record" your desktop, or even stream it!

It includes some helper "record your screen capture/stream it" and setup utilities.
It also includes a helper (virtual-audio-capturer) to record "what you hear/see on your desktop" for vista+ system.

  In order to use those you'll want/need the java JRE installed on your system first.

It also includes a free, general purpose, open source directshow desktop/screen source capture filter.

You can use this with any (directshow compatible) program to capture or stream, like VLC, etc.

See also the ChangeLog.txt file for a list of changes in latest versions.

== Installation ==

Download installer and run, from 

  https://sourceforge.net/projects/screencapturer/files
    (sorry the files are on sourceforge: github limits their disk usage space and I'm hitting up against the limit)
	
== Audio capture ==

It also includes a directshow source capture filter device for recording "what you hear" in windows 7/vista--any sound card!
  see https://github.com/rdp/virtual-audio-capture-grabber-device for info/troubleshooting of that audio capture device.

== Usage ==

Use some helper programs provided in 
 Start menu -> Programs -> Screen Capture Recorder -> record/broadcast/XXX

Or use any 3rd party program that can read from a Directshow Capture Device (ex: VLC).

VLC example: http://betterlogic.com/roger/2010/07/how-to-use-vlc-as-a-free-open-source-alternative-to-playon-tv

ffmpeg example (requires ffmpeg 32 bit):

  $ ffplay -f dshow -i video="screen-capture-recorder"

or

  $ ffmpeg -f dshow  -i video="screen-capture-recorder"  -r 20 -t 10 screen-capture.mp4 # -t 10 for 10 seconds recording

or combine it with recording "what you hear" audio [vista/windows 7] (using ffmpeg in this example):

  $ ffmpeg -f dshow -i audio="virtual-audio-capturer":video="screen-capture-recorder" yo.mp4
  
The audio device name can be any other dshow audio devices, as well.

Avisynth also works with it.  Add it to a filter graph using graphedit, then use DirectShowSource as your 
input source with that graphedit filename specified.

gstreamer works, as well, ex:

  $ gst-launch.exe dshowvideosrc device-name=screen-capture-recorder ! ffmpegcolorspace ! directdrawsink

I've even had Skype accidentally use it, thinking it was my webcam.
Let me know if you want an easier way made for it for anything.

== Configuration ==

By default, it captures the "full screen" of the main desktop monitor (all windows, overlapping, from there, with aero if vista+, without transparent windows if non aero).

To configure it differently, run the provided "configuration setup utilities/shortcuts" or 
adjust registry settings before starting a run (advanced users only):

HKEY_CURRENT_USER\Software\screen-capture-recorder

with DWORD keys respected of start_XXX etc ... 
(see the included file {installdir}\configuration_setup_utility\setup_screen_tracker_params.rb for the full list of registry key values
available, or see https://github.com/rdp/screen-capture-recorder-to-video-windows-free/blob/master/configuration_setup_utility/setup_screen_tracker_params.rb#L9 )
  
ex: see configuration_setup_utility\incoming.reg file (though NB that those values are in hex, so editing that file is a bit tedious--
I always just use regedit or the accompanying script utilities and don't edit it by hand).  

To "reset" a value delete its key.

== Trouble shooting ==

it's too slow! (ex: 15 fps)
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
it's too slow! (ex: 50 fps)
  disabling or enabling "hardware acceleration" for the desktop might help, too, or setting it to one stop above none
  try switching from 32 to 24 bit or 16 bit input (from this), and/or switching your desktop depth itself--or both! fastest is 16 bit display with capture in 16 bit mode--400 fps whoa!).  

It starts making the computer sluggish after awhile.
  Make sure you have enough RAM available (virtualbox is known to hog some without telling you).  Also disable aero if possible.
  
  
Note that if your output is, say, going to be 10 fps "actually used" in the end, ex:
$ ffmpeg -f dshow -i video=video-capture-recorder -r 10 yo.mp4 -r 10 # output file is only 10 fps

Then to save cpu, a "good" application will also instruct the "source fps" to be 10 fps
$ ffmpeg -framerate 10 -f dshow -i video=video-capture-recorder -r 10 yo.mp4 # input and output are set to 10

for an exaplantion of why "framerate" and not "-r" here, see https://trac.ffmpeg.org/wiki/DirectShow#Specifyinginputframerate

that they pass on to this filter, and everybody's in 10 fps and hopefully happy, and cpu is saved.
ex: vlc.exe dshow:// ... :dshow-fps=10
vlc then passes this in to SetFormat after negotiation, and the device accepts it, and feeds you at 10 fps.

Some "bad" directshow applications don't pass this parameter in.  Most can.
So there may be a case where it is wasting cpu somehow, by capturing too many or the like.
So for those programs, you'll want to set the max fps parameter, which will limit its
capture frequency.

Setting max fps to greater than 30 also allows you to get fps greater than 30 (the default max).  It "enables" them,
by giving it a higher max default.  I didn't think people would normally care/want them so there you have.

Skype users: NB that to use it as an input camera skype, you need to specify that it is a reasonably small capture, see
http://betterlogic.com/roger/2012/04/skype-directshow-device-just-shows-circles-for-the-preview-capture/

Q. I can't capture full screen [like starcraft type windows].
Known limitation, try this thread: https://groups.google.com/forum/#!topic/roger-projects/uxmGV_vW4iY [or pay me a bit of money and I'll create a front end to the excellent OBS project to provide this functionality]

== Feedback/Question ==

Other feedback/problems/questions ping me roger-projects@googlegroups.com or browse the mailing list: http://groups.google.com/group/roger-projects
Or submit to our uservoice: http://rdp.uservoice.com

==  Redistribution ==

You can redistribute this.  See file LICENSE.TXT.
If you do, you may just need to distribute for instance the file PushDesktop.dll
If you do, you have to register this device, via registering its dll 
(like $ regsvr32 PushDesktop.dll--may require administrator rights) as part of your install.
Also note that end users also need the MSVC 2010 redistributable previously installed (or you have to make it available 
in the same dir as your exe) for the dll to work/install, in case end users don't have it installed).
If you change it and redistribute it, you may want to change the GUID's, so that it won't clash if
somebody also installs this software.
See also https://github.com/rdp/screen-capture-recorder-to-video-windows-free/issues/17

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

== Related ==

http://obsproject.com is open source, doesn't use FFmpeg but might work for you.  May even capture DirectX woot.
http://www.ffsplit.com is not open source, but does capture/stream desktop using FFmpeg.
http://camstudio.org is open source and free, but requires you to just use your local codecs installed in your windows
  system, and has some usability issues, which is why I even started this project in the first place :)
