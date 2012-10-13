require 'java'
require 'add_vendored_gems_to_load_path'

splash = java.awt.SplashScreen.splash_screen
splash.close if splash # close it early'ish...not sure how this looks on slower computers...

require 'jruby-swing-helpers/lib/simple_gui_creator'
require 'ffmpeg_helpers'
include SimpleGuiCreator # ::DropDownSelector, etc.


VirtualAudioDeviceName = 'virtual-audio-capturer'
ScreenCapturerDeviceName = 'screen-capture-recorder' 
