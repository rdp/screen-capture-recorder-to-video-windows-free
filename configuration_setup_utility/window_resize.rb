require 'jruby-swing-helpers/mouse'
require 'jruby-swing-helpers/swing_helpers'
require 'setup_screen_tracker_params'
require 'java'

class MouseDraw
  
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
        if setting %2 == 1 and [:width, :height].include?(key)
          SwingHelpers.show_blocking_message_dialog "warning #{key} is an odd number, which won't record for mplayer/ffmpeg\nso rounding it up for you..."
          setting = setting + 1
        end
        begin
          setter_getter.set_single_setting key, setting
        rescue Exception => e
          p e, e.to_s, e.backtrace
        end
      end
      SwingHelpers.show_blocking_message_dialog 'done setting them to match that window. Details:' + "\n" + got.inspect
      f.dispose
    }
    current_values = setter_getter.all_current_values
    f.set_size(current_values['width'] || 200, current_values['height'] || 200)
    f.set_location(current_values['start_x'] || 0, current_values['start_y'] || 0)
    AWTUtilities.set_window_opacity(f, 0.6)
  	f.setDefaultCloseOperation(JFrame::EXIT_ON_CLOSE) # instead of hang when they click the "X" [LODO warn?]
    f.show
  end
  
end

if $0 == __FILE__
  MouseDraw.go
end