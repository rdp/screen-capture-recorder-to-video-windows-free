require 'add_vendored_gems_to_load_path'
p $:
require 'java'

require 'simple_gui_creator'
p $LOADED_FEATURES
Storage
SwingHelpers = SimpleGuiCreator
include SwingHelpers # DropDownSelector

# and others
require 'tempfile'
require 'sane'
LocalStorage = Storage.new('server_setup')

if ARGV[0].in? ['-h', '--help']
  puts "syntax: [--redo-with-last-run]"
  exit 0
end

require 'os'

if OS.doze?
require 'win32/registry'

vlc_dir = nil
for key in [Win32::Registry::HKEY_LOCAL_MACHINE, Win32::Registry::HKEY_CURRENT_USER]
  for subkey in ['Software\\VideoLAN\\VLC', 'Software\Wow6432Node\\VideoLAN\\VLC'] # allow for 64-bit java, 32-bit VLC [yipers oh my]
    break if vlc_dir
    begin
      reg = key.open(subkey)
      if vlc_dir = reg['InstallDir']
        #p 'success', key, subkey
      end
    rescue => e
      #p 'unable to open reg?' + key.to_s # happens somehow...
    ensure
      reg.close if reg
    end
  end
end
if !vlc_dir
  SwingHelpers.show_blocking_message_dialog 'unable to determine where VLC is installed, using default [might not work]'
  vlc_dir = "c:\\program files\\videolan\\vlc"
end

else
  vlc_dir = "/Users/rogerdpack/Downloads/VLC.app/Contents/MacOS"
end

ENV['PATH'] = vlc_dir + File::PATH_SEPARATOR + ENV['PATH']
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
extension = '/go.mp3'

if (!is_port_open(port)) 
  SwingHelpers.show_blocking_message_dialog "Cannot start server, close any other open instances of VLC\nPort #{port} seems already in use..."
  exit 1
end

local_ip_addrs = Socket.get_local_ips

if ARGV[0] == "--redo-with-last-run"
  # lodo, port check might be poor, if we ever have it variable
  old_ip = LocalStorage['ip_previously_selected']
  SwingHelpers.show_blocking_message_dialog "ip address may have changed, or it was never configured yet\nPlease rerun original audio broadcaster config, and go through all step completely to set it up." unless old_ip.in? local_ip_addrs 
  c = LocalStorage['last_version']
  assert c.present?
  Thread.new { system c + " --qt-start-minimized" }
  dialog = SwingHelpers.show_blocking_message_dialog "started vlc server minimized...access remotely via #{LocalStorage['url_to_access_it']}"
  exit 0
end

if OS.doze?
  require 'ffmpeg_helpers'
  names = FFmpegHelpers.enumerate_directshow_devices[:audio]
  SwingHelpers.show_blocking_message_dialog "You will first be prompted for the audio device you wish to capture and stream.\nFor Vista/Windows 7 users:\n    choose virtual-audio-capturer\nFor XP:\n    you'll need to configure your recording device to record stereo mix (a.k.a waveout mix or record what you hear). Google it and set it up first."
  name = DropDownSelector.new(nil, names, "Select audio device to capture and stream").go_selected_value
  name = "dshow:// :dshow-vdev=none :dshow-adev=\"#{name}\""
else
  name = "qtsound://"
end

popup = SwingHelpers.show_non_blocking_message_dialog 'starting audio server...'
vlc_thread = Thread.new {
  c =%!vlc #{name} --sout "#transcode{acodec=mp3,ab=128}:standard{access=http,mux=raw,dst=:#{port}#{extension}}" ! # --qt-start-minimized  
  print c
  LocalStorage['last_version'] = c
  system c # we let this go forever
}

sleep 1 # let VLC startup...though it takes a bit longer than this at times :P...
popup.close

SwingHelpers.show_blocking_message_dialog "Server started (VLC) on :#{port}#{extension}. You can minimize VLC if you wish."

if(JOptionPane.show_select_buttons_prompt("Would you like to test your setup, or just continue running server?", :yes => "test server setup", :no => "just run the server") == :no)
 b = SwingHelpers.show_non_blocking_message_dialog "ok, exiting, it should be running, just leave VLC running...you can minimize it if desired..."
 sleep 2.5
 b.close
 exit 0
end


SwingHelpers.show_blocking_message_dialog "First let's test if I can read from the stream locally...you'll hear some blips, which you should ignore, they are being broadcast and then recorded...."

# LODO you are successfully capturing what you hear (not even streaming it yet...)
# describe making sure mute is turned off on the client.

def play_sound_and_capture_and_test_playback ip_addr_to_listen_on, port

  out = Tempfile.new 'wav' 
  out = out.path + '.wav'

  p = PlayAudio.new 'alerts.wav' # 5s long
  p.start

  c ="ffmpeg -t 2 -y -i http://#{ip_addr_to_listen_on}:#{port}/go.mp3 \"#{out}\""
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
    p 'unable to delete?' + out + ' ' + ignore.to_s
  end
  got
end

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

p 'this computers ip addrs:' + local_ip_addrs.join(',')
if local_ip_addrs.length == 1
  ip = local_ip_addrs.first
else
  ip = DropDownSelector.new(nil, local_ip_addrs, "Select IP address that you think the client will be most likely to see:").go_value
end

SwingHelpers.show_blocking_message_dialog "ok now let's test for interfering firewalls by seeing if we can read, locally, from our own IP address #{ip}, ignore these first blips again..."
LocalStorage['ip_previously_selected'] = ip
LocalStorage['url_to_access_it'] = "http://#{ip}:#{port}#{extension}"

heard = play_sound_and_capture_and_test_playback ip, port

if heard == :no
 SwingHelpers.show_blocking_message_dialog "maybe you have a firewall that's disallowing connections? try turning it off, try again..."
 exit 1
end

SwingHelpers.show_blocking_message_dialog "Good, hopefully this means local firewalls are disabled.\nNow let's double check that.\nGo to your client computer, open a command prompt, and run \"ping #{ip}\"\nYou should see outputs like\n\nReply from #{ip}: bytes=32 time=27ms TTL=53 ..."


got = JOptionPane.show_select_buttons_prompt "Does ping work from client to server?", :yes => "yes", :no => "no"
if got == :no
  message = "try disabling more firewalls, on client and server"
  message += " or try with selecting a different IP address" if local_ip_addrs.length > 1
  SwingHelpers.show_blocking_message_dialog message
end

good_addr = "http://#{ip}:#{port}#{extension}"
RubyClip.set_clipboard good_addr
SwingHelpers.show_blocking_message_dialog "Good you're theoretically done! Ready to go!  Now go to your client (receiving) machine,\nRun VLC -> Media Menu -> Open Network Streame\n and type in\n#{good_addr}\nIt might also work to alternatively type in\nhttp://#{Socket.gethostname}:#{port}#{extension}\nIt should connect, and audio from this computer play on that one.\nLeave VLC running on the server computer, though it can be minimized.\n#{good_addr} has been copied to your local clipboard."
