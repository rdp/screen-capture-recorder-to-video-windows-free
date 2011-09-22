require 'jruby-swing-helpers/swing_helpers'
include SwingHelpers
require 'java'

names = ['one, two, three']
p DropDownSelector.new(nil, names, "Select audio device to stream:").go

#transcode{vcodec=none,acodec=mp3,ab=128,channels=2,samplerate=44100}'