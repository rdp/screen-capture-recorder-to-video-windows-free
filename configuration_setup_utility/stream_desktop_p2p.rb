template=
%!-------------Stream Desktop--------------
"Output url, like udp://236.0.0.1:2000:name"
[udp://:stream_url,width=700, height=20px]
[                                        ]
[Start/Stop:start_stop_button] "status:status_text"
!

# XXXheight=1char should work here...

puts template
require 'common_recording.rb'

@status = :stopped

@frame = ParseTemplate.new.parse_setup_string template
@frame.elements[:start_stop_button].on_clicked {
  if @status == :stopped
     c = "ffmpeg -f dshow  -framerate 5 -i video=screen-capture-recorder -vf scale=1280:720 -vcodec libx264 -pix_fmt yuv420p -tune zerolatency -preset ultrafast -qp 10 -g 30 -f mpegts #{frame.elements[:stream_url].text}"
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

update_ui

# XXX
#%!
#-------------Stream Desktop--------------
#"Output url, like udp://236.0.0.1:2000"
# should not have an extra blank line at the top...
