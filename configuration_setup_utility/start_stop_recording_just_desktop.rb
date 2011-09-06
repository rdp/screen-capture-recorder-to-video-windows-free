require 'jruby-swing-helpers/swing_helpers'
# use like ".mp3" "audio=xxx" "ffplay"
# if desired
ENV['PATH'] = 'ffmpeg\bin;' + ENV['PATH']
include SwingHelpers

device = ARGV[1] || "video=screen-capture-recorder -r 20"
exe = ARGV[2] || "ffmpeg"

if exe == "ffmpeg"
  seconds = SwingHelpers.get_user_input("Seconds to record for?", 60)
  seconds = "-t #{seconds}"
  p "doing #{seconds} seconds"

  file = JFileChooser.new_nonexisting_filechooser_and_go 'select_file_to_write_to', File.dirname(__FILE__)
  file += (ARGV[0] || ".mp4" ) unless file =~ /\./ # add extension for them...
  if File.exist? file
    got = JOptionPane.show_select_buttons_prompt "overwrite #{file}?", :yes => "yes", :no => "cancel"
    raise unless got == :yes
    File.delete file
  end
else
  raise unless exe == "ffplay"
end

#got = JOptionPane.show_select_buttons_prompt 'Select start to start', :yes => "start", :no => "stop"
#raise unless got == :yes

c = "#{exe} -f dshow -i #{device} #{seconds} #{'"' + file + '"' if file}"
puts c
p c
system c
if exe == "ffmpeg"
  p 'revealing...'
  SwingHelpers.show_in_explorer file
  p 'done'
end