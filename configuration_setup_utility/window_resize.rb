require './add_vendored_gems_to_load_path.rb'
require 'jruby-swing-helpers/mouse'
require 'jruby-swing-helpers/swing_helpers'
require 'setup_screen_tracker_params'
require 'java'

class WindowResize
  
  def self.go
    java_import 'javax.swing.JFrame'
    java_import 'javax.swing.JButton'
    java_import 'com.sun.awt.AWTUtilities'
    JFrame.setDefaultLookAndFeelDecorated(true); # allow full window transparency for jdk 7
  
    f = JFrame.new
    f.bring_to_front
    SwingHelpers.show_blocking_message_dialog "resize the next window to be directly over your desired area, then click in the middle to save"
    button = JButton.new('CLICK HERE WHEN DONE TO SET')
    f.add button
    setter_getter = SetupScreenTrackerParams.new
    button.add_action_listener {|e|
      setter_getter.re_init
      got = {:start_x => f.get_location.x, :start_y => f.get_location.y, :width => f.get_size.width, :height => f.get_size.height}
      for key, setting in got
  	    for name, factor in {'mplayer/ffmpeg' => 2, 'vlc' => 8}
          if setting % factor != 0 and [:width, :height].include?(key)
            SwingHelpers.show_blocking_message_dialog "warning #{key} is not divisible by #{factor}, which won't record for #{name}\nso rounding it up for you..."
            setting = (setting/factor*factor) + factor # round up phew!
		        got[key] = setting # for the english output at the end
          end
        end
        setter_getter.set_single_setting key, setting
      end
      SwingHelpers.show_blocking_message_dialog 'done setting them to match that window. Details:' + "\n" + got.inspect
      f.dispose
  }
  current_values = setter_getter.all_current_values
	width = current_values['width'] || 200
	height = current_values['height'] || 200
	min_val = 35
	if(width < min_val) 
	  SwingHelpers.show_blocking_message_dialog 'previous width too small, using default size '+ min_val.to_s
	  width = min_val
	 end
	 if height < min_val
	  SwingHelpers.show_blocking_message_dialog 'previous height too small, using default size ' + min_val.to_s
	  height = min_val
	 end
	 
    f.set_size(width, height)
    f.set_location(current_values['start_x'] || 0, current_values['start_y'] || 0)
    AWTUtilities.set_window_opacity(f, 0.7)
  	f.setDefaultCloseOperation(JFrame::EXIT_ON_CLOSE) # instead of hang when they click the "X" [LODO warn?]
    f.show
  end
  
end

if $0 == __FILE__
  WindowResize.go
end