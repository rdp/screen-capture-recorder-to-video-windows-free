require 'java'
require 'add_vendored_gems_to_load_path'

splash = java.awt.SplashScreen.splash_screen
splash.close if splash # close it early'ish...not sure how this looks on slower computers...

require File.dirname(__FILE__) + '/jruby-swing-helpers/swing_helpers'
include SwingHelpers
require 'ffmpeg_helpers'

VirtualAudioDeviceName = 'virtual-audio-capturer'

def choose_devices
  videos = ['none'] + FfmpegHelpers.enumerate_directshow_devices[:video].sort_by{|name| name == 'screen-capture-recorder' ? 0 : 1}  # put none in front :)
  original_video_device=video_device = DropDownSelector.new(nil, videos, "Select video device to capture, or \"none\" to record audio only").go_selected_value
  if video_device == 'none'
    video_device = nil
  else
    # stay same
  end

  audios = ['none'] + FfmpegHelpers.enumerate_directshow_devices[:audio].sort_by{|name| name == VirtualAudioDeviceName ? 0 : 1}
  audio_device = DropDownSelector.new(nil, audios, "Select audio device to capture, or none").go_selected_value
  if audio_device == 'none'
    audio_device = nil 
  else
    # stay same
  end
  
  unless audio_device || video_device
   SwingHelpers.show_blocking_message_dialog('must select at least one')
   exit 1
  end
  
  if original_video_device == 'screen-capture-recorder'
    SwingHelpers.show_blocking_message_dialog "you can setup parameters [like frames per second, size] for the screen capture recorder\n in its separate setup configuration utility"
  end

  [audio_device, video_device] 
end

def combine_devices_for_ffmpeg_input audio_device, video_device
 if audio_device
   audio_device="audio=\"#{audio_device}\""
 end
 if video_device
   video_device="video=\"#{video_device}\""
 end
 "-f dshow -i #{[audio_device,video_device].compact.join(':')}"
end