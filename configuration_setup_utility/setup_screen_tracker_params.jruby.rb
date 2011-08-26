# to run this use c:\>ruby setup_params.rb 1024 800 ... (doesn't require jruby)
# or jruby setup_params.rb to have it prompt you for values and set them (requires jruby).

require 'win32/registry'

# returns blank if none set...
def get_user_input(message, default = '', cancel_ok = false)
  received = javax.swing.JOptionPane.showInputDialog(message, default)
  received
end

class SetupScreenTrackerParams
  Settings = ['height', 'width', 'start_x', 'start_y', 'fps']
 
  def initialize
    @screen_reg = Win32::Registry::HKEY_CURRENT_USER.create "Software\\os_screen_capture" # LODO .keys fails?
  end
  
  def set_single_setting name, value
    raise unless Settings.include?(name)
    raise unless value.is_a? Fixnum
    @screen_reg.write(name, Win32::Registry::REG_DWORD, value.to_i)
  end
  
  # can be nil if not set...
  def read_single_setting name
    @screen_reg[name] rescue nil
  end
  
  def teardown
    @screen_reg.close
    @screen_reg = nil
  end
  
end


def do_command_line
  setter = SetupScreenTrackerParams.new
  for type in SetupScreenTrackerParams::Settings
    previous_setting = setter.read_single_setting type
    if ARGV.index('--just-display-current-settings')
      puts "#{type}=#{previous_setting}"
      next
    end
    received = ARGV.shift
    unless received
      require 'java' # jruby only for getting user input...
      previous_setting = '' if previous_setting == 0
      received = get_user_input('enter desired ' + type + ' (blank to reset to default)', previous_setting)
      raise 'canceled...remaining settings have not been changed, but previous ones were' unless received
    end
    if received == ''
      received = 0 # 0 is interpreted as "use defaults" when read from the registry, in the C code
    else
      received = received.to_i
    end
    if received % 2 == 1 # it's odd
      p "warning--odd numbers were given, but they are rejected from mplayer for some reason!"
    end
    setter.set_single_setting type, received
    puts "set #{type} => #{received}"
  end
  setter.teardown
  p 'done setting them all'
end


if $0 == __FILE__
   
  if ARGV.detect{|a| ['-h', '--help'].include? a}
    puts "syntax: [--just-display-current-settings] or #{SetupScreenTrackerParams::Settings.join(' ')} or nothing to be prompted for the various settings/values"
    exit 0
  end
  do_command_line
end