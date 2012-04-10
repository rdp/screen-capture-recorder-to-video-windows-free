require 'setup_screen_tracker_params.rb'
require 'add_vendored_gems_to_load_path.rb' # for swinghelpers' sane

# no delete functionality in this one...
def do_command_line
  setter = SetupScreenTrackerParams.new
  for type in SetupScreenTrackerParams::Settings
    p type
    previous_setting = setter.read_single_setting type
    if ARGV.index('--just-display-current-settings') || ARGV.index('--just')
      puts "#{type}=#{previous_setting}"
      next
    end
    received = given_from_command_line = ARGV.shift
    unless received
      require 'java' # jruby only for getting user input...
      require 'jruby-swing-helpers/swing_helpers'
      previous_setting ||= ''
      received = SwingHelpers.get_user_input('enter desired ' + type + ' (blank resets it to the default [full screen, 30 fps, primary monitor]', previous_setting, true)
      raise 'cancelled...remaining settings have not been changed, but previous ones to this one were...' unless received # it should at least be the empty string...
    end
    if received == '' # allow "empty" input to mean "reset this"
      setter.delete_single_setting type
      p 'deleted ' + type.to_s
      next
    else
      received = received.to_i
    end
    if (received % 2 == 1) && ['height', 'width'].include?(type)
      warning= "warning--odd numbers were chosen, but videos with odd number width/height are rejected by mplayer for some reason, so be careful [setting it anyway]!"
      p warning
      begin
	    # allow MRI to use it [?]
        require 'java'
        require 'jruby-swing-helpers/swing_helpers'
        SwingHelpers.show_blocking_message_dialog warning
      rescue LoadError => allow_mri_for_setting_from_cli
      end
    end
    setter.set_single_setting type, received
    puts "set #{type} => #{received}"
  end
  p 'done setting them all' unless ARGV.index('--just-display-current-settings')
end


if $0 == __FILE__
   
  if ARGV.detect{|a| ['-h', '--help'].include? a}
    puts "syntax: [--just-display-current-settings] or #{SetupScreenTrackerParams::Settings.join(' ')} or nothing to be prompted for the various settings/values"
    exit 0
  end
  do_command_line
  if ARGV.index('--just-display-current-settings')
    p 'hit enter to continue' # pause :P
    STDIN.getc
  end

end