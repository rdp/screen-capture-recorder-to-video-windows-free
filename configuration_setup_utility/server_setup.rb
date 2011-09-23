require 'add_vendored_gems'
require 'jruby-swing-helpers/swing_helpers'
require 'jruby-swing-helpers/play_audio'
include SwingHelpers
require 'java'
require 'tmpdir'
require 'sane' # IP addrs

ENV['PATH'] = 'vendor\\ffmpeg\\bin;' + ENV['PATH'] 

out = `ffmpeg.exe -list_devices true -f dshow -i dummy 2>&1`
audio = out.split('DirectShow')[2]
names = audio.scan(/"([^"]+)"/).map{|matches| matches[0]}

name = DropDownSelector.new(nil, names, "Select audio device to stream:").go_selected_value

# test port is open at all...
require 'socket'
port = 8081
a = TCPServer.new nil, port
a.close

popup = SwingHelpers.show_non_blocking_message_dialog 'starting server...'

vlc_thread = Thread.new {
  c =%!vlc dshow:// :dshow-vdev=none :dshow-adev=\"#{name}\" --sout "#transcode{acodec=mp3,ab=128}:standard{access=http,mux=mpeg,dst=:#{port}/go.mp3}" ! # --qt-start-minimized  

  print c
  system c
}

sleep 1
popup.close

def play_sound_and_capture_and_playback ip_addr_to_listen_on, port

  out = Dir.tmpdir + 'out.wav'
  require 'fileutils'
  FileUtils.touch out # writable?

  p = PlayAudio.new 'alerts.wav' # 5s long
  p.start

  c ="ffmpeg.exe -t 1 -y -i http://#{ip_addr_to_listen_on}:#{port}/go.mp3 \"#{out}\""
  print c
  system c # takes 5s...yipes LODO report how cruddy it is LOL.
  p.stop

  SwingHelpers.show_blocking_message_dialog 'now I will play back what I recorded from that stream. You\'ll tell me if you hear anything...'
  p = PlayAudio.new out
  p.start
  sleep 4
  p.stop
  got = JOptionPane.show_select_buttons_prompt "did you hear anything?", :yes => "yes", :no => "no"
  # maybe playaudio doesn't release its hold?
  # File.delete out
  got
end

SwingHelpers.show_blocking_message_dialog 'now let\'s test if I can read from the stream locally...you\'ll hear some blips, which you should ignore, as they are put on the stream, broadcast, then received locally and saved to a file.'
got = play_sound_and_capture_and_playback '127.0.0.1', port
if got == :yes
 p 'yes'
else
  SwingHelpers.show_blocking_message_dialog 'please check VLC\'s log messages:\n Go to VLC, hit the stop button, go to tools -> messages, change verbosity to 2, \nthen without closing the messages window, hit the play button, \nthen report back the log message'
end

SwingHelpers.show_blocking_message_dialog 'ok now we\'ll try with what we think is your IP address'

local_ip_addrs = Socket.get_local_ips # hope it works :)
p local_ip_addrs
if local_ip_addrs.length == 1
  ip = local_ip_addrs.first
else
  ip = DropDownSelector.new(nil, local_ip_addrs, "Select IP address that you think the client will be most likely to see:").go_value
end

SwingHelpers.show_blocking_message_dialog 'ok now let\'s test if we can read, locally, from our own IP address #{ip} (the server streaming to itself from itself)...'

heard = play_sound_and_capture ip, port

if heard == :yes
 SwingHelpers.show_blocking_message_dialog 'good you\'re theoretically ready to broadcast.  Now turn off any firewalls on the server, go to your client (receiving) machine\n, start VLC ->\nMedia Menu\nOpen Capture Device\n and type in "http://#{ip}:#{port}/go.mp3.  It might also work with http://#{Socket.gethostname}:#{port}/go.mp3'
else
 SwingHelpers.show_blocking_message_dialog "maybe you have a firewall that's disallowing connections?"
end