require 'jruby-swing-helpers/swing_helpers'
include SwingHelpers
require 'java'

names = ['one, two, three']
p DropDownSelector.new(nil, names, "Select audio device to stream:").go

#transcode{vcodec=none,acodec=mp3,ab=128,channels=2,samplerate=44100}'

# t: dshow://
# qt4 debug: Adding option: dshow-vdev=none
# qt4 debug: Adding option: dshow-adev=Line In (High Definition Audio
# qt4 debug: Adding option: dshow-caching=200
# qt4 debug: Adding option: :sout=#transcode{vcodec=none,acodec=mp3,ab=128,channels=2,samplerate=44100}