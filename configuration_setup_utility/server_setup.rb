puts 'loading...'
require 'add_vendored_gems'
require 'jruby-swing-helpers/swing_helpers'
require 'jruby-swing-helpers/play_audio'
require 'jruby-swing-helpers/ruby_clip'
include SwingHelpers
require 'java'
require 'tempfile'
require 'sane'

ENV['PATH'] = 'vendor\\ffmpeg\\bin;' + ENV['PATH'] 
ffmpeg_list_command = "ffmpeg.exe -list_devices true -f dshow -i dummy 2>&1"
out = `#{ffmpeg_list_command}`
unless out.present?
  p 'failed', out
  raise `2nd try: #{ffmpeg_list_command}`
end

audio = out.split('DirectShow')[2]
raise out.inspect unless audio
names = audio.scan(/"([^"]+)"/).map{|matches| matches[0]}

SwingHelpers.show_blocking_message_dialog "You will first be prompted for the audio device you wish to capture and stream.\nFor Vista/Windows 7 users:\n    choose virtual-audio-capturer\nFor XP:\n    you'll need to configure your recording device to record stereo mix (a.k.a waveout mix or record what you hear). Google it and set it up first."
name = DropDownSelector.new(nil, names, "Select audio device to capture and stream").go_selected_value

def is_port_open port
 begin
  require 'socket'
  a = TCPServer.new nil, port
  a.close
  true
 rescue Exception => e
  p e.to_s
  false
 end
end  

port = 8888

if (!is_port_open(port)) 
  SwingHelpers.show_blocking_message_dialog "Cannot start server, close open instances of VLC?\nPort #{port} seems already in use..."
  exit 1
end

require 'win32/registry'
for key in [Win32::Registry::HKEY_LOCAL_MACHINE, Win32::Registry::HKEY_CURRENT_USER]
  reg = key.open('Software\\VideoLAN\\VLC')
  if vlc_loc = reg['InstallDir']
    break
  end
end
vlc_loc ||= "c:\\program files\\videolan\\vlc"
ENV['PATH'] = vlc_loc + ';' + ENV['PATH']

popup = SwingHelpers.show_non_blocking_message_dialog 'starting server...'
vlc_thread = Thread.new {
  # TODO needs full path...
  c =%!vlc dshow:// :dshow-vdev=none :dshow-adev=\"#{name}\" --sout "#transcode{acodec=mp3,ab=128}:standard{access=http,mux=raw,dst=:#{port}/go.mp3}" ! # --qt-start-minimized  
  # mux ogg?
  print c
  system c
}

sleep 1 # let VLC startup...though it takes a bit longer than this...
popup.close

# LODO you are successfully capturing what you hear (not even streaming it yet...)
# describe making sure mute is turned off on the client.

def play_sound_and_capture_and_test_playback ip_addr_to_listen_on, port

  out = Tempfile.new 'wav' # LODO delete ja
  out = out.path + '.wav'

  p = PlayAudio.new 'alerts.wav' # 5s long
  p.start

  c ="ffmpeg.exe -t 2 -y -i http://#{ip_addr_to_listen_on}:#{port}/go.mp3 \"#{out}\""
  system c
  p.stop

  SwingHelpers.show_blocking_message_dialog 'now I will play back what I recorded from that. You\'ll then tell me if you hear anything...'
  p = PlayAudio.new out
  p.start
  sleep 4
  p.stop
  got = JOptionPane.show_select_buttons_prompt "did you hear any beeps just now?", :yes => "yes", :no => "no"
  # maybe playaudio doesn't release its hold?
  begin
    File.delete out
  rescue => ignore
    p ignore.to_s
  end
  got
end

SwingHelpers.show_blocking_message_dialog "Server started (VLC). You can minimize it if you wish.\nNow let's test if I can read from the stream locally...you'll hear some blips, which you should ignore, they are being broadcast and then recorded...."

#if (is_port_open(port)) # windows let's one supplant the other...hmm...
#  SwingHelpers.show_blocking_message_dialog "Cannot start VLC as server?"
#  exit 1
#end


got = play_sound_and_capture_and_test_playback '127.0.0.1', port
if got == :no
  SwingHelpers.show_blocking_message_dialog "please check VLC\'s log messages:\n Go to VLC, hit the stop button, go to tools -> messages, change verbosity to 2, \nthen without closing the messages window, hit the play button, \nthen report back the log message"
  exit
end

SwingHelpers.show_blocking_message_dialog "ok that worked! This means you are successfully capturing \'what you hear\' and streaming it.\n Now we'll try receiving it from a more \"public\" IP address to check for any blocking local firewalls."

local_ip_addrs = Socket.get_local_ips # hope it works :)
p local_ip_addrs
if local_ip_addrs.length == 1
  ip = local_ip_addrs.first
else
  ip = DropDownSelector.new(nil, local_ip_addrs, "Select IP address that you think the client will be most likely to see:").go_value
end

SwingHelpers.show_blocking_message_dialog "ok now let's test for interfering firewalls by seeing if we can read, locally, from our own IP address #{ip}, ignore these first blips again..."

heard = play_sound_and_capture_and_test_playback ip, port

if heard == :no
 SwingHelpers.show_blocking_message_dialog "maybe you have a firewall that's disallowing connections? try turning it off, try again..."
 exit 1
end

SwingHelpers.show_blocking_message_dialog "Good, hopefully this means local firewalls are disabled.\nNow let's double check that.\nGo to your client computer, open a command prompt, and run \"ping #{ip}\"\nYou should see outputs like\n\nReply from #{ip}: bytes=32 time=27ms TTL=53 ..."


got = JOptionPane.show_select_buttons_prompt "did ping work from client to server?", :yes => "yes", :no => "no"
if got == :no
  message = "try disabling more firewalls, on client and server"
  message += " or try with selecting a different IP address" if local_ip_addrs.length > 1
  SwingHelpers.show_blocking_message_dialog message
end

good_addr = "http://#{ip}:#{port}/go.mp3"
RubyClip.set_clipboard good_addr
SwingHelpers.show_blocking_message_dialog "Good you're theoretically done! Ready to go!  Now go to your client (receiving) machine,\nRun VLC -> Media Menu -> Open Capture Device\n and type in\n#{good_addr}\nIt might also work to alternatively type in\nhttp://#{Socket.gethostname}:#{port}/go.mp3\nIt should connect, and audio from this computer play on that one.\nLeave VLC running on the server computer, though it can be minimized.\n#{good_addr} has been copied to your local clipboard."
