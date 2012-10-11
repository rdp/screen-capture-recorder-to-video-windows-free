require 'java'
require 'add_vendored_gems_to_load_path'

splash = java.awt.SplashScreen.splash_screen
splash.close if splash # close it early'ish...not sure how this looks on slower computers...

require 'jruby-swing-helpers/lib/simple_gui_creator'

include SimpleGuiCreator # ::DropDownSelector, etc.
require 'ffmpeg_helpers'

VirtualAudioDeviceName = 'virtual-audio-capturer'
ScreenCapturerDeviceName = 'screen-capture-recorder' 

def combine_devices_for_ffmpeg_input audio_device, video_device # NB not for ffplay input!! won't work with ffplay but it should shouldn't it?
 if audio_device
   audio_device="-f dshow -i audio=\"#{FfmpegHelpers.escape_for_input audio_device}\""
 end
 if video_device
   video_device="-f dshow -i video=\"#{FfmpegHelpers.escape_for_input video_device}\""
 end
 "#{video_device} #{audio_device}"
end