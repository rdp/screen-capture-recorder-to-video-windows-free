# TODO: end ffmpeg on exit yikes!

template=
%!-------------Stream Desktop--------------
"Enter your output streaming url, like udp://236.0.0.1:2000 (mplayer will receive from the same address):"
"   udp://hostname:port:fake_name"
"   or for multicast something like udp://236.0.0.1:2000:"
"  You can receive the stream via some player, ex::"
" mplayer -demuxer +mpegts -framedrop -benchmark udp://236.0.0.1:2000:"
[udp://localhost:2000:stream_url,width=700, height=20px]
[                                                      ]
[Start/Stop:start_stop_button] "status:status_text,width=50chars"
!

# XXX height=1char should work better here...sigh

puts template
require 'common_recording.rb'
@storage = Storage.new('stream_desktop_p2p')
@storage.set_default('stream_to_url', 'udp://236.0.0.1:2000')
@status = :stopped

@frame = ParseTemplate.new.parse_setup_string template
@frame.elements[:stream_url].text=@storage['stream_to_url']
@frame.elements[:start_stop_button].on_clicked {
  if @status == :stopped
       extra_options="-qp 10 -g 30"
      #s extra_options="-g 30"
       c = "ffmpeg -f dshow  -framerate 5 -i video=screen-capture-recorder -vf scale=1280:720 -vcodec libx264 -pix_fmt yuv420p -tune zerolatency -preset ultrafast #{extra_options} -f mpegts #{@frame.elements[:stream_url].text.strip}"
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
    # already running, send it a quit command, let it clean itself up
    @current_process.puts 'q' rescue nil
  end
}

def update_ui  
  @frame.elements[:status_text].text="status:#{@status}" 
end

update_ui # init

@frame.after_closed {
  if @status == :running
    SimpleGuiCreator.show_blocking_message_dialog "warning, shutting down streaming with window closing"
   end
   @storage['stream_to_url']=@frame.elements[:stream_url].text

}