require 'setup_screen_tracker_params.rb'
require 'add_vendored_gems_to_load_path.rb' # for swinghelpers' sane

def do_setup_via_numbers
  setter = SetupScreenTrackerParams.new
  for type in SetupScreenTrackerParams::Settings
    previous_setting = setter.read_single_setting type
    if ARGV.index('--just-display-current-settings') || ARGV.index('--just')
      puts "#{type}=#{previous_setting}"
      next
    else
      p "previously had been set to [#{previous_setting}]"
    end
    received = ARGV.shift # if they want the CLI version :)
    unless received
      require 'java' # jruby only for getting user input...
      require 'jruby-swing-helpers/lib/simple_gui_creator'
      previous_setting ||= ''
      received = SimpleGuiCreator.get_user_input('enter desired value for:' + type + ' (leave blank to reset to defaults)', previous_setting, true)
    end
	p "got #{received}"
	if !received
	  p "skipping"
	  next
	end
    if received == "" # allow "empty" input to mean "reset this"
      setter.delete_single_setting type
      p 'deleted ' + type.to_s
      next
    else
	  if received =~ /^0x/
	    received = received.to_i(16) # 0x009004
	  elsif received =~ /\./
	    raise 'cannot use floats, at least today'
	  else
        received = received.to_i
	  end
    end
    if (received % 2 == 1) && ['height', 'width'].include?(type)
      warning= "warning--odd numbers were chosen, but videos with odd number width/height are rejected by mplayer for some reason, so be careful [setting it anyway]!"
      p warning
      begin
	    # allow MRI to use it [?]
        require 'java'
        require 'jruby-swing-helpers/lib/swing_helpers'
        SimpleGuiCreator.show_blocking_message_dialog warning
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
  do_setup_via_numbers
  if ARGV.index('--just-display-current-settings')
    p 'hit enter to continue' # pause :P
    STDIN.getc
  end

end