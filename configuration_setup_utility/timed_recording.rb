puts 'loading...'

require 'add_vendored_gems'

require 'jruby-swing-helpers/swing_helpers'
include SwingHelpers
require 'jruby-swing-helpers/drive_info'
require 'setup_screen_tracker_params'

if !File.exist?(ENV['windir'] + "\\system32\\msvcr100.dll")
  SwingHelpers.show_blocking_message_dialog "warning--you don't appear to have the Microsoft visual studio 2010 runtime DLL's installed, please install them first and then run the \"re-register device\" command."
  SwingHelpers.open_url_to_view_it_non_blocking "http://www.microsoft.com/download/en/details.aspx?id=5555"
  exit 1
end

file = JFileChooser.new_nonexisting_filechooser_and_go 'Select Filename for output file', DriveInfo.get_drive_with_most_space_with_slash
file += (ARGV[0] || ".avi" ) unless file =~ /\.avi$/ # force extension on them...
if File.exist? file
  got = JOptionPane.show_select_buttons_prompt "overwrite #{file}?", :yes => "yes", :no => "cancel"
  raise unless got == :yes
  File.delete file
end

# LODO tell them current size/settings here

old_fps = SetupScreenTrackerParams.new.read_single_setting("force_max_fps") || 30 # our own local default...hmm...
new_fps = SwingHelpers.get_user_input("desired capture speed (frames per second) [more means more responsive, but requires more cpu/disk]", old_fps).to_i

seconds = SwingHelpers.get_user_input("Seconds to record for?", 60)

SwingHelpers.show_blocking_message_dialog "the recording (#{seconds}s) will start a bit after you click ok."

#got = JOptionPane.show_select_buttons_prompt 'Select start to start', :yes => "start", :no => "stop"
#raise unless got == :yes

c = "ffmpeg -f dshow -i video=screen-capture-recorder -r #{new_fps} -t #{seconds} -vcodec huffyuv \"#{file}\""
puts c
system c
p 'revealing file...'
SwingHelpers.show_in_explorer file
p 'done'
