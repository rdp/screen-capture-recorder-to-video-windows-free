require 'add_vendored_gems_to_load_path'

require 'jruby-swing-helpers/swing_helpers'
include SwingHelpers
require 'jruby-swing-helpers/drive_info'
require 'setup_screen_tracker_params'
require 'fileutils'

require 'ffmpeg_helpers'

def choose_devices
  videos = FfmpegHelpers.enumerate_directshow_devices[:video].sort_by{|name| name == 'screen-capture-recorder' ? 0 : 1} + ['none'] # put none in front :)
  original_video_device=video_device = DropDownSelector.new(nil, videos, "Select video device to capture, or \"none\" to record audio only").go_selected_value
  if video_device == 'none'
    video_device = nil
  else
    video_device="-f dshow -i video=\"#{video_device}\""
  end

  audios = ['none'] + FfmpegHelpers.enumerate_directshow_devices[:audio].sort_by{|name| name == 'virtual-audio-capturer' ? 0 : 1}
  audio_device = DropDownSelector.new(nil, audios, "Select audio device to capture, or none").go_selected_value
  if audio_device == 'none'
    audio_device = nil 
  else
    audio_device="-f dshow -i audio=\"#{audio_device}\""
  end
  
  unless audio_device || video_device
   SwingHelpers.show_blocking_message_dialog('must select at least one')
   exit 1
  end
  
  if original_video_device == 'screen-capture-recorder'
    SwingHelpers.show_blocking_message_dialog "you can setup lots of parameters [like frames per second, size] for the screen capture recorder\n in its separatesetup configuration utility"
  end

  [audio_device, video_device] 
end

