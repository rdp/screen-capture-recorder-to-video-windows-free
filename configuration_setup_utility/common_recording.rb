require 'java'
require 'add_vendored_gems_to_load_path' # also sets up FFmpeg bin dir to PATH

splash = java.awt.SplashScreen.splash_screen
splash.close if splash # close it early'ish...not sure how this looks on slower computers...

require 'jruby-swing-helpers/lib/simple_gui_creator'
require 'ffmpeg_helpers'
include SimpleGuiCreator # ::DropDownSelector, etc.

def show_message m
  SimpleGuiCreator.show_message m
end

VirtualAudioDeviceName = 'virtual-audio-capturer'
ScreenCapturerDeviceName = 'screen-capture-recorder' 
