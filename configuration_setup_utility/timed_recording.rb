# bundled gems
for dir in Dir[File.dirname(__FILE__) + "/vendor/*/lib"]
  $: << dir
end

require 'jruby-swing-helpers/swing_helpers'
include SwingHelpers
require 'jruby-swing-helpers/drive_info'
require 'setup_screen_tracker_params'

# use vendored ffmpeg
ENV['PATH'] = File.dirname(__FILE__) + '\vendor\ffmpeg\bin;' + ENV['PATH']

old_fps = SetupScreenTrackerParams.new.read_single_setting("max_fps") || 10
device = "video=screen-capture-recorder -r #{old_fps}" # -r xxx or it never stops... hmm...
exe = "ffmpeg"

file = JFileChooser.new_nonexisting_filechooser_and_go 'select_file_to_write_to', DriveInfo.get_drive_with_most_space_with_slash
file += (ARGV[0] || ".mp4" ) unless file =~ /\.mp4$/ # force extension on them...
if File.exist? file
  got = JOptionPane.show_select_buttons_prompt "overwrite #{file}?", :yes => "yes", :no => "cancel"
  raise unless got == :yes
  File.delete file
end
seconds = SwingHelpers.get_user_input("Seconds to record for?", 60)

SwingHelpers.show_blocking_message_dialog "starting the recording (#{seconds}s) after you close this dialog..."

#got = JOptionPane.show_select_buttons_prompt 'Select start to start', :yes => "start", :no => "stop"
#raise unless got == :yes

c = "#{exe} -f dshow -i #{device} -t #{seconds} #{'"' + file + '"' if file}"
puts c
system c
p 'revealing...'
SwingHelpers.show_in_explorer file
p 'done'
