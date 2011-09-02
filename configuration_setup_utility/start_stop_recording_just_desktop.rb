require 'swing_helpers'

include SwingHelpers

got = JOptionPane.show_select_buttons_prompt 'Select start to start', :yes => "start", :no => "stop"

if got == :yes
  file = FileDialog.new_previously_existing_file_selector_and_go 'select_file_to_write_to'
  seconds = SwingHelpers.get_user_input("seconds to record for?")
  c = "ffmpeg -f dshow -i video=screen-capture-recorder -t #{seconds} \"#{file}\""
  puts c
  puts `#{c}`
else
  p 'canceled'
end
  