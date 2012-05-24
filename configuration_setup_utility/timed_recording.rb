require 'common_recording.rb'
require 'fileutils'
require 'jruby-swing-helpers/lib/drive_info'
 
audio_device, video_device = choose_devices

# choose a filename 'before hand' [lame-o, I know]

if video_device
    possible_extensions = ['avi', 'mpg']
else
    possible_extensions = ['mp3', 'wav']
end

file = JFileChooser.new_nonexisting_filechooser_and_go 'Select Filename for output file', DriveInfo.get_drive_with_most_space_with_slash

unless file.split('.')[-1].in? possible_extensions
  file += '.' + possible_extensions[0] # force an extension for them...
  SwingHelpers.show_blocking_message_dialog "Selected file #{file}"
end

if File.exist? file
  got = JOptionPane.show_select_buttons_prompt "overwrite #{file}?", :yes => "yes", :no => "cancel"
  raise unless got == :yes
  FileUtils.touch file # make sure it's writable
end

seconds = SwingHelpers.get_user_input("Seconds to record for?", 60)

SwingHelpers.show_blocking_message_dialog "the recording (#{seconds}s) will start a little bit after you click ok."

#got = JOptionPane.show_select_buttons_prompt 'Select start to start', :yes => "start", :no => "stop"
#raise unless got == :yes

c = "ffmpeg -y #{combine_devices_for_ffmpeg_input(audio_device, video_device)} -t #{seconds} -vcodec huffyuv \"#{file}\"" # LODO report qtrle no video
puts c
system c
puts # get past any ffmpeg output
puts 'revealing file...'
SwingHelpers.show_in_explorer file
p 'done'
