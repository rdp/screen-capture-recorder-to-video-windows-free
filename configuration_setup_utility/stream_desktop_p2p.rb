mplayer_options = "-demuxer +mpegts -framedrop -benchmark"

template=
%!-------------Stream Desktop--------------
"Enter your output streaming url, like udp://236.0.0.1:2000 (mplayer will receive from the same address):"
"   udp://specific_hostname:port:fake_ui_name"
"   or for multicast something like udp://236.0.0.1:2000:fake_ui_name2"
"   or more exotic options FFmpeg accepts, like udp://236.0.0.1:2000?pkt_size=500:fake_ui_name4"
"  You can receive the stream via some player, ex:"
" mplayer #{mplayer_options} ffmpeg://udp://236.0.0.1:2000?fifo_size=1000000&buffer_size=1000000:fake_ui_name3"
[udp://localhost:2000:stream_url,width=600, height=20px]
[                                                      ]
 "status:status_text,width=100chars" 

 [Start Normal (5 fps):start_stop_button]
 [Start variable q:start_no_q]
 [Start variable q more iframes:start_all_iframes]
 [Start all raw:start_all_raw]
 [Start Normal 25 fps:start_all_25_fps]
 [Start Normal 15 fps:start_all_15_fps]
 [Start Normal 10 fps:start_all_10_fps]
 [Stop Streaming:stop_streaming]
 [launch mplayer receiver:launch_mplayer] "After you start streaming"

 !
# XXX height=1char should work here, but it doesn't quite enough...sigh
#     XXX editable=true
# XXX not need fake_ui_name...

puts template
require 'common_recording.rb'
@storage = Storage.new('stream_desktop_p2p')
@storage.set_default('stream_to_url', 'udp://236.0.0.1:2000')
@status = :stopped

@frame = ParseTemplate.new.parse_setup_string template
@frame.elements[:stream_url].text=@storage['stream_to_url']
{:start_stop_button => "-qp 10 -g 30", :start_no_q => "-g 30", :start_all_iframes => "-g 5"}.each{|button_name, extra_options|
  @frame.elements[button_name].on_clicked {
   start_stop_ffmpeg extra_options
  }
}

@frame.elements[:start_all_raw].on_clicked {
  start_stop_ffmpeg "", "-vcodec copy" # no need for -g here... :)
}

{:start_all_25_fps => 25, :start_all_15_fps => 15, :start_all_10_fps => 10}.each{|button_name, fps|
  @frame.elements[button_name].on_clicked {
    start_stop_ffmpeg "-g 30 -qp 10", nil, fps
  }
}

@frame.elements[:launch_mplayer].on_clicked {
  # buffer size is kernel's SO_RECVBUF size, set to 64k by default, probably doesn't matter here anyway...
  # fifo_size
  fifo_size=10_000_000/188 # docs say to pass it in divided by 188...
  c = "mplayer.exe #{mplayer_options} ffmpeg://#{get_current_url.split('?')[0]}?fifo_size=#{fifo_size}&buffer_size=1000000"
  puts "running #{c}"
  Thread.new { 
	system(c) # hangs if no streaming has started...	
  }
}

@current_style=''

def update_ui  
  if @status == :running
    @frame.elements[:status_text].text="status:#{@status} #{@current_style} #{get_current_url}"
  else
    @frame.elements[:status_text].text="status:#{@status}"
  end
  # don't mess with launch_mplayer grey'ing out here in case run on different box
end

update_ui # init

@frame.after_closed {
  if @status == :running
    SimpleGuiCreator.show_blocking_message_dialog "warning, shutting down without stopping streamingso we will kill ffmpeg now"
	stop_ffmpeg
   end
   @storage['stream_to_url'] = @frame.elements[:stream_url].text

}

def get_current_url
  @frame.elements[:stream_url].text.strip
end

@frame.elements[:stop_streaming].on_clicked {
  stop_ffmpeg
}

def start_stop_ffmpeg extra_options, vcodec=nil, framerate = 5
 vcodec ||= "-vcodec libx264 -pix_fmt yuv420p -tune zerolatency -preset ultrafast"
 if @status == :stopped  
       c = "ffmpeg -f dshow  -framerate #{framerate} -i video=screen-capture-recorder -vf scale=1280:720 #{vcodec} #{extra_options} -f mpegts #{get_current_url}"
	   puts "starting #{c}"
	   @current_process = IO.popen(c, "w") # jruby friendly :P
	   Thread.new { 
		 @status = :running
		 @current_style = "framerate #{framerate}"
		 update_ui
		 # handle potential early outs...
		 FFmpegHelpers.wait_for_ffmpeg_close @current_process # won't raise, no second parameter XXX catch raise
		 @status = :stopped
		 update_ui
	   }
  else
    stop_ffmpeg
  end
end

def stop_ffmpeg
    puts "stopping running ffmpeg"
    # already running, send it a quit command, let it clean itself up [should I pause here?]
    @current_process.puts 'q' rescue nil
end