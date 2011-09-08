# use like ".mp3" "audio=xxx" "ffplay"
# if desired

require 'jruby-swing-helpers/swing_helpers'
# bundled gems
for dir in Dir[File.dirname(__FILE__) + "/vendor/*/lib"]
  $: << dir
end
require 'jruby-swing-helpers/drive_info'


ENV['PATH'] = File.dirname(__FILE__) + '\vendor\ffmpeg\bin;' + ENV['PATH']
include SwingHelpers

device = ARGV[1] || "video=screen-capture-recorder -r 20" # -r 20 or it never stops... hmm...
exe = ARGV[2] || "ffmpeg"

if exe == "ffmpeg"

  file = JFileChooser.new_nonexisting_filechooser_and_go 'select_file_to_write_to', DriveInfo.get_drive_with_most_space_with_slash
  file += (ARGV[0] || ".mp4" ) unless file =~ /\./ # add extension for them...
  if File.exist? file
    got = JOptionPane.show_select_buttons_prompt "overwrite #{file}?", :yes => "yes", :no => "cancel"
    raise unless got == :yes
    File.delete file
  end
  seconds = SwingHelpers.get_user_input("Seconds to record for?", 60)
  seconds = "-t #{seconds}"
  p "starting #{seconds} seconds now"
else
  raise unless exe == "ffplay"
end

#got = JOptionPane.show_select_buttons_prompt 'Select start to start', :yes => "start", :no => "stop"
#raise unless got == :yes

c = "#{exe} -f dshow -i #{device} #{seconds} #{'"' + file + '"' if file}"
puts c
system c
if exe == "ffmpeg"
  p 'revealing...'
  SwingHelpers.show_in_explorer file
  p 'done'
end