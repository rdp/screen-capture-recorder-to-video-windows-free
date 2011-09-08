require 'jruby-swing-helpers/mouse'
require 'setup_screen_tracker_params'

class MouseDraw
  
  def self.go
  
  require 'java'

  java_import 'javax.swing.JFrame'
  java_import 'javax.swing.JButton'
  java_import 'com.sun.awt.AWTUtilities'

  f = JFrame.new
  f.add JButton.new('capture window')
  f.set_size(200,200)

  AWTUtilities.set_window_opacity(f, 0.5)
  
  # wait till mouse goes down...
  print 'drag mouse to draw'
  while(Mouse.left_mouse_button_state == :up)
    sleep 0.05
  end
  f.undecorated = true
  f.default_close_operation = JFrame::EXIT_ON_CLOSE
  f.always_on_top = true
  f.visible = true
  start_x, start_y = Mouse.get_mouse_location
  f.set_location(start_x, start_y)
  while(Mouse.left_mouse_button_state == :down)
    # set_size
    p 'waiting for drag to end'
    x, y = Mouse.get_mouse_location
    width = [x-start_x, 20].max
    height = [y-start_y, 20].max
    f.set_size width, height
    #sleep 0
  end
  f.dispose
  {:start_x => start_x, :start_y => start_y, :width => width, :height => height}
end
  
end

if $0 == __FILE__
  got = MouseDraw.go
  p got
  setter = SetupScreenTrackerParams.new
  for key, setting in got
    setter.set_single_setting key, setting
  end
  p 'done setting'
end