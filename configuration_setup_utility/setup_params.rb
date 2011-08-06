# to run this use c:\>ruby setup_params.rb 1024 800 ... (doesn't require jruby)
# or jruby setup_params.rb to have it prompt you for values and set them (requires jruby).

require 'win32/registry'

# returns blank if none set...
def get_user_input(message, default = '', cancel_ok = false)
  received = javax.swing.JOptionPane.showInputDialog(message, default)
  received
end

settings = ['height', 'width', 'start_x', 'start_y']

screen_capture = Win32::Registry::HKEY_CURRENT_USER.create "Software\\os_screen_capture" # LODO .keys fails?
if ARGV.detect{|a| ['-h', '--help'].include? a}
  puts "syntax: [--just-display-current-settings] or #{settings.join(' ')}"
  exit 0
end

for type in settings
  previous_setting = screen_capture[type]
  if ARGV.index('--just-display-current-settings')
    puts "#{type}=#{previous_setting}"
    next
  end
  received = ARGV.shift
  unless received
    require 'java' # jruby only from here on out...
    received = get_user_input('enter desired ' + type + ' (blank for reset to default)', previous_setting)
    raise 'canceled...remaining settings have not been changed, but previous ones were' unless received
  end
  if received == ''
    received = 0 # reset...
  else
    received = received.to_i
  end
  if received % 2 == 1 # it's odd
    p "warning--odd numbers were given, but they are rejected from mplayer for some reason!"
  end
  screen_capture.write(type, Win32::Registry::REG_DWORD, received.to_i)
  puts "set #{type} => #{received}"
end
screen_capture.close
p 'done setting them all'