# TODO: end ffmpeg on exit

template=
%!-------------Stream Desktop--------------
"Enter your output streaming url, like udp://236.0.0.1:2000 (mplayer will receive from the same address):"
"   udp://specific_hostname:port:fake_ui_name"
"   or for multicast something like udp://236.0.0.1:2000:fake_ui_name2"
"  You can receive the stream via some player, ex:"
" mplayer -demuxer +mpegts -framedrop -benchmark ffmpeg://udp://236.0.0.1:2000?fifo_size=1000000:fake_ui_name3"
[udp://localhost:2000:stream_url,width=700, height=20px]
[                                                      ]
 "status:status_text,width=50chars"

 [Start/Stop Normal:start_stop_button]
 [Start/Stop variable q setting:start_no_q]
 [Start/Stop variable q more iframes:start_all_iframes]
 [Start/Stop all raw:start_all_raw]
 [Start/Stop Normal high fps:start_all_high_fps]

 !
# XXX height=1char should work better here...sigh

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

@frame.elements[:start_all_high_fps].on_clicked {
  start_stop_ffmpeg "-g 30 -qp 10", nil, 25
}


def update_ui  
  @frame.elements[:status_text].text="status:#{@status}" 
end

update_ui # init

@frame.after_closed {
  if @status == :running
    SimpleGuiCreator.show_blocking_message_dialog "warning, shutting down without stopping streaming, which means you'll have to kill ffmpeg manually"
   end
   @storage['stream_to_url'] = @frame.elements[:stream_url].text

}

def start_stop_ffmpeg extra_options, vcodec=nil, framerate = 5
 vcodec ||= "-vcodec libx264 -pix_fmt yuv420p -tune zerolatency -preset ultrafast"
 if @status == :stopped  
       c = "ffmpeg -f dshow  -framerate #{framerate} -i video=screen-capture-recorder -vf scale=1280:720 #{vcodec} #{extra_options} -f mpegts #{@frame.elements[:stream_url].text.strip}"
	   puts "starting #{c}"
	   @current_process = IO.popen(c, "w") # jruby friendly :P
	   Thread.new { 
		 @status = :running
		 update_ui
		 # handle potential early outs...
		 FFmpegHelpers.wait_for_ffmpeg_close @current_process # won't raise, no second parameter XXX catch raise
		 @status = :stopped
		 update_ui
	   }
  else
    puts "stopping running ffmpeg"
    # already running, send it a quit command, let it clean itself up
    @current_process.puts 'q' rescue nil
  end
end