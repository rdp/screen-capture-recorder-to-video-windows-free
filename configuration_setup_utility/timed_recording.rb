puts 'loading...'

require 'add_vendored_gems'

require 'jruby-swing-helpers/swing_helpers'
include SwingHelpers
require 'jruby-swing-helpers/drive_info'
require 'setup_screen_tracker_params'
require 'fileutils'

if !File.exist?(ENV['windir'] + "\\system32\\msvcr100.dll")
  SwingHelpers.show_blocking_message_dialog "warning--you don't appear to have the Microsoft visual studio 2010 runtime DLL's installed, please install them first and then run the \"re-register device\" command."
  SwingHelpers.open_url_to_view_it_non_blocking "http://www.microsoft.com/download/en/details.aspx?id=5555"
  exit 1
end

require 'ffmpeg_helpers'
videos = FfmpegHelpers.enumerate_directshow_devices[:video].sort_by{|name| name == 'screen-capture-recorder' ? 0 : 1} + ['none'] # put ours in front :)
original_video_device=video_device = DropDownSelector.new(nil, videos, "Select video device to capture, or none").go_selected_value
if video_device == 'none'
  video_device = nil
else
  video_device="-f dshow -i video=\"#{video_device}\""
end

audios = FfmpegHelpers.enumerate_directshow_devices[:audio].sort_by{|name| name == 'virtual-audio-capturer' ? 0 : 1} + ['none']
audio_device = DropDownSelector.new(nil, audios, "Select audio device to capture, or none").go_selected_value
if audio_device == 'none'
  audio_device = nil 
else
  audio_device="-f dshow -i audio=\"#{audio_device}\""
end

if video_device
  possible_extensions = ['avi', 'mpg']
else
  possible_extensions = ['mp3', 'wav']
end

unless audio_device || video_device
 SwingHelpers.show_blocking_message_dialog('must select at least one')
 exit 1
end

file = JFileChooser.new_nonexisting_filechooser_and_go 'Select Filename for output file', DriveInfo.get_drive_with_most_space_with_slash
unless file.split('.')[-1].in? possible_extensions
  file += '.' + possible_extensions[0] # force extension on them...
  SwingHelpers.show_blocking_message_dialog "Selected file #{file}"
end

if File.exist? file
  got = JOptionPane.show_select_buttons_prompt "overwrite #{file}?", :yes => "yes", :no => "cancel"
  raise unless got == :yes
  FileUtils.touch file
end

if original_video_device == 'screen-capture-recorder'
  # LODO tell them current size/settings here
  old_fps = SetupScreenTrackerParams.new.read_single_setting("force_max_fps") || 30 # our own local default...hmm...
  new_fps = SwingHelpers.get_user_input("desired capture speed (frames per second) [more means more responsive, but requires more cpu/disk]", old_fps).to_i
  new_fps = "-r #{new_fps}"
end

seconds = SwingHelpers.get_user_input("Seconds to record for?", 60)

SwingHelpers.show_blocking_message_dialog "the recording (#{seconds}s) will start a little bit after you click ok."

#got = JOptionPane.show_select_buttons_prompt 'Select start to start', :yes => "start", :no => "stop"
#raise unless got == :yes

c = "ffmpeg -y #{[video_device, audio_device].compact.join(' ')} #{new_fps} -t #{seconds} -vcodec huffyuv \"#{file}\""
puts c
system c
p 'revealing file...'
SwingHelpers.show_in_explorer file
p 'done'
