# coding: UTF-8
require 'common_recording.rb'
require 'thread'

frame = ParseTemplate.new.parse_setup_filename 'template.record_buttons'
frame.elements[:start].disable
frame.elements[:stop].disable
@frame = frame

@storage = Storage.new('record_with_buttons_v6') # bump this when stored settings format changes :)
def storage
  @storage
end

require 'tmpdir'
storage['save_to_dir'] ||= Dir.tmpdir

storage.set_default('should_record_to_file', true)
storage.set_default('reveal_files_after_each_recording', true) # otherwise it confuses the heck out of me...what's it doing? huh wuh? 

def current_storage_dir
  @storage['save_to_dir']
end

elements = frame.elements

@elements = elements

def elements
  @elements
end

elements[:reveal_save_to_dir].on_clicked {
  last_filename = get_old_files.last
  if last_filename
    SimpleGuiCreator.show_in_explorer last_filename
  else
    SimpleGuiCreator.show_blocking_message_dialog "none have been recorded yet, so revealing the directory they will be recorded to"
    SimpleGuiCreator.show_in_explorer current_storage_dir	
  end
}

def get_old_files
  old_files = Dir[current_storage_dir + '/*.{wav,mp4,mkv,ac3,mp3,mpg,mov}']  
  old_files.select!{|f| File.basename(f) =~ /^\d+\..../}
  old_files = old_files.sort_by{|f| f =~ /(\d+)\....$/; $1.to_i}
  old_files
end

def should_stream?
  storage['should_stream'] && storage[:url_stream].present?
end

def should_save_file?
  storage['should_record_to_file']
end

def get_title
  device_names = [video_device, audio_device].map{|name, idx| name}.compact
  if device_names.length == 2
    orig_names = device_names
    device_names = device_names.map{|name| name.size > 7 ? name[0..7] + "..." : name}.join(', ')
  else
    # leave as is...
	device_names = device_names[0]
  end
  title = 'Will record to: '
  destinos = []
  next_file_basename = File.basename(@next_filename)
  destinos << File.basename(File.dirname(@next_filename)) + '/' + next_file_basename if should_save_file?
  destinos << storage[:url_stream].split(':')[0] if should_stream?
  title += destinos.join(', ')
  title += " from #{device_names}"
end

def setup_ui
  numbered = get_old_files.map{ |f| f =~ /(\d+)\....$/; $1.to_i}
  next_number = (numbered[-1] || 0) + 1
  ext = storage['current_ext_sans_dot']
  @next_filename = "#{current_storage_dir}/#{next_number}.#{ext}"
  
  @frame.title = get_title
  
  if(@current_process)
    elements[:stop].enable 
    elements[:start].disable
    elements[:start].text = "Recording!"
  else
    elements[:stop].disable
    elements[:start].text = "Start!"
    elements[:start].enable
  end
  if !should_save_file? && !should_stream?
    elements[:stop].disable
    elements[:start].text = "Set options 1st!"
    elements[:start].disable
  end
  
end

@process_input_mutex = Mutex.new

def process_input_mutex
  @process_input_mutex
end

elements[:start].on_clicked {
 if @current_process
   raise 'unexpected double start?'
 else
   if video_device && storage['show_transparent_window_first'] # don't display window if not recording video
     require 'window_resize'
     elements[:start].disable
     window = WindowResize.go false, false, 'CLICK HERE WHEN READY TO START RECORDING'
     window.after_closed { start_recording_with_current_settings }
   else
     start_recording_with_current_settings
   end
 end
}

@frame.after_closed {
 if @current_process
   SimpleGuiCreator.show_blocking_message_dialog "an ffmpeg instance was left running, will close it for you..."
   elements[:stop].click! # just in case :P
 end
}

def start_recording_with_current_settings just_preview = false

   unless storage['video_name'] || storage['audio_name']
     SimpleGuiCreator.show_blocking_message_dialog('must select at least one of video or audio') # just in case...
     return
   end
   
   if storage['resolution'].present?
     resolution = "-s #{storage['resolution']}"
   end

   if storage['video_name']
     pixel_type = "yuv420p"
    # pixel_type = "yuv444p"
	 
     codecs = "-vcodec libx264 -pix_fmt #{pixel_type} #{resolution} -preset ultrafast -vsync vfr -acodec libmp3lame" # qtrle was 10 fps, this kept up at 15 on dual core, plus is .mp4 friendly, though lossy, looked all right
   else
     prefix_to_acodec = {'mp3' => '-acodec libmp3lame -ac 2', 'aac' => '-acodec aac -strict experimental', 'wav' => '-acodec pcm_s16le'}
     audio_type = prefix_to_acodec[storage['current_ext_sans_dot']]
	 raise 'unknown prefix?' unless audio_type
     codecs = audio_type
   end
   
   if storage['stop_time'].present?
     stop_time = "-t #{storage['stop_time']}"     
   end
   
   ffmpeg_commands = "#{FFmpegHelpers.combine_devices_for_ffmpeg_input storage['audio_name'], storage['video_name'] } #{codecs}"
   if just_preview
     # doesn't take audio, lame...
	 if !storage['video_name']
	    SimpleGuiCreator.show_blocking_message_dialog('you only have audio, this button only previews video for now, ping me to have it improved...') # just in case...
        return
	 end
	 # lessen volume in case of feedback...
	 # analyzeduration 0 to make it pop up quicker...
     c = "ffmpeg #{ffmpeg_commands} -af volume=0.75 -f flv - | ffplay -analyzeduration 0 -f flv -" # couldn't get multiple dshow, -f flv to work... lodo try ffplay with lavfi...maybe?
	 puts c
	 system c
	 puts 'done preview' # clear screen...
	 return
   end
   
   # panic here causes us to not see useful x264 error messages...
   c = "ffmpeg -loglevel info #{stop_time} #{ffmpeg_commands} -f mpegts - | ffmpeg -f mpegts -i -"
   
   if should_save_file?	 
	 c += " -c copy \"#{@next_filename}\""
     puts "writing to #{@next_filename}"
   end
   
   if should_stream?
     c += " -c copy -f flv #{storage[:url_stream]}"
	 puts "streaming..."
   end
   puts "about to run #{c}"
   @current_process = IO.popen(c, "w") # jruby friendly :P
   Thread.new { 
     # handle potential early outs...
     FFmpegHelpers.wait_for_ffmpeg_close @current_process # won't raise, no second parameter XXX catch raise
	 elements[:stop].click!
   }
   setup_ui
   @frame.title = "Recording " + get_title
end

elements[:stop].on_clicked {
  process_input_mutex.synchronize {
    if @current_process
      # .close might "just kill" ffmpeg, and skip trailers in certain muxes, so tell it to shutdown gracfully
      @current_process.puts 'q' rescue nil # can fail, meaning I guess ffmpeg already exited...
	  FFmpegHelpers.wait_for_ffmpeg_close @current_process	  
	  @current_process = nil
      # @current_process.close
      puts # pass the ffmpeg stuff, hopefully
      puts "done writing #{@next_filename}"
      if storage['reveal_files_after_each_recording'] 
	    if File.exist? @next_filename
          SimpleGuiCreator.show_in_explorer(@next_filename)
		end
      end      
    else
      # could be they had a timed recording, and clicked stop
    end
	setup_ui # refresh in case error occurred somehow...
  }
}

def reset_options_frame
  @options_frame.close
  setup_ui # reset the main frame too :)
  # don't show options frame still, it just feels so annoying...
  # elements[:preferences].click!
end

def remove_quotes string
  string.gsub('"', '')
end

def shorten string
  if string && string.size > 0
    string[0..20] + '...'
  else
    ""
  end
end

elements[:preferences].on_clicked {
  template = <<-EOL
  ------------ Recording Options -------------
  [Select video device:select_new_video] " #{remove_quotes(video_device_name_or_nil || 'none selected')} :video_name"
  [Select audio device:select_new_audio] " #{remove_quotes(audio_device_name_or_nil || 'none selected')} :audio_name" 
  [✓:record_to_file] "Save to file"   [ Set file options :options_button]
  [✓:stream_to_url_checkbox] "Stream to url:"  "Specify url first!:url_stream_text" [ Set streaming url : set_stream_url ]
  "Stop recording after this many seconds:" "#{storage['stop_time']}" [ Click to set :stop_time_button]
  "Current record resolution: #{storage['resolution'] || 'native (input resolution)'} :fake" [Change :change_resolution]
  [Preview current settings:preview] "a rough preview of how the recording will look"
  [ Close Options Window :close]
  EOL
  print template
  # TODO it can automatically 'bind' to a storage, and automatically 'always call this method for any element after clicked' :)
  
  @options_frame = ParseTemplate.new.parse_setup_string template
  frame = @options_frame
  if storage['should_record_to_file']
    frame.elements[:record_to_file].set_checked!
  else
    frame.elements[:record_to_file].set_unchecked!
  end
  frame.elements[:record_to_file].on_clicked { |new_value|
    storage['should_record_to_file'] = new_value
	reset_options_frame
  }
  
  if storage['should_stream']
    frame.elements[:stream_to_url_checkbox].set_checked!
  else
    frame.elements[:stream_to_url_checkbox].set_unchecked!
  end
  frame.elements[:stream_to_url_checkbox].on_clicked {|new_value|
    storage['should_stream'] = new_value
    reset_options_frame
  }
  
  if !storage[:url_stream].present?
    frame.elements[:stream_to_url_checkbox].set_unchecked!
    frame.elements[:stream_to_url_checkbox].disable! # can't check it if there's nothing to use...
  else
    frame.elements[:url_stream_text].text = shorten(storage[:url_stream])
  end
  
  frame.elements[:set_stream_url].on_clicked {
    stream_url = SimpleGuiCreator.get_user_input "Url to stream to, like rtmp://live....", storage[:url_stream], true
    storage[:url_stream] = stream_url
	reset_options_frame
  }
  
  frame.elements[:preview].on_clicked {
    start_recording_with_current_settings true
  }
  
  frame.elements[:select_new_video].on_clicked {
    choose_video
	reset_options_frame
  }
  
  frame.elements[:select_new_audio].on_clicked {
    choose_audio
	reset_options_frame
  }
  
  frame.elements[:change_resolution].on_clicked { 
    storage['resolution'] = DropDownSelector.new(nil, ['native', 'vga', 'svga', 'hd480', 'hd720', 'hd1080'], "Select resolution").go_selected_value
    storage['resolution'] = nil if storage['resolution']  == 'native' # :)
	reset_options_frame
  }
  
  frame.elements[:close].on_clicked { frame.close }

  frame.elements[:stop_time_button].on_clicked {  
    stop_time = SimpleGuiCreator.get_user_input "Automatically stop the recording after a certain number of seconds (leave blank and click ok for it to record till you click the stop button)", storage['stop_time'], true
    storage['stop_time'] = stop_time
	reset_options_frame
  }

  frame.elements[:options_button].on_clicked {  
    storage['save_to_dir'] = SimpleGuiCreator.new_existing_dir_chooser_and_go 'select save to dir', current_storage_dir

    if SimpleGuiCreator.show_select_buttons_prompt("Would you like to automatically display files in windows explorer after recording them?") == :yes
      storage['reveal_files_after_each_recording'] = true
    else
      storage['reveal_files_after_each_recording'] = false
    end
	reset_options_frame
  }
}

def bootstrap_devices

	if(!video_device && !audio_device)

	  need_help = false
	  if FFmpegHelpers.enumerate_directshow_devices[:audio].include?([VirtualAudioDeviceName, 0])
		# some reasonable defaults :P
		storage['audio_name'] = [VirtualAudioDeviceName, 0]
		storage['current_ext_sans_dot'] = 'mp3'
	  else
		need_help = true
	  end
	  
	  # *my* preferred defaults for combined video/audio :)
	  if ARGV[0] != '--just-audio-default'
		if FFmpegHelpers.enumerate_directshow_devices[:video].include?([ScreenCapturerDeviceName, 0])
		  storage['video_name'] = [ScreenCapturerDeviceName, 0]
		  storage['current_ext_sans_dot'] = 'mp4'  
		else
		  need_help = true
		end
	  end
	  elements[:preferences].simulate_click if need_help
	else
	  FFmpegHelpers.warmup_ffmpeg_so_itll_be_disk_cached # why not? my fake attempt at making ffmpeg realtime startup fast LOL
	end

end

def choose_media type
  # put virtuals at top of the list :)
  # XXX put currently selected at top?
  media_options = FFmpegHelpers.enumerate_directshow_devices[type]
  # no sort_by! in 1.9 mode TODO chagned generic_run_rb.bat to 1.9 :)
  media_options = media_options.sort_by{|name, idx| (name == VirtualAudioDeviceName || name == ScreenCapturerDeviceName) ? 0 : 1}
  names = ['none'] + media_options.map{|name, idx| name}
  idx = DropDownSelector.new(nil, names, "Select #{type} device to capture, or none").go_selected_idx
  if idx == 0
    device = nil # reset to none
  else
    device = media_options[idx - 1]    
  end
  device
end

def choose_video
  video_device = choose_media :video  
  storage['video_name'] = video_device
  
  if video_device_name_or_nil == ScreenCapturerDeviceName
    SimpleGuiCreator.show_blocking_message_dialog "you can setup parameters [like frames per second, size] for the screen capture recorder\n in its separate setup configuration utility"
      if SimpleGuiCreator.show_select_buttons_prompt("screen capture recorder: Would you like to display a resizable setup window before each recording?") == :yes
        storage['show_transparent_window_first'] = true
      else
        storage['show_transparent_window_first'] = false
      end
  end  
  choose_extension
end

def choose_audio
  audio_device = choose_media :audio
  storage['audio_name'] = audio_device
  choose_extension
end

def audio_device 
  storage['audio_name']
end

def video_device 
  storage['video_name']
end

def audio_device_name_or_nil
  storage['audio_name'] ? storage['audio_name'][0] : nil
end

def video_device_name_or_nil
  storage['video_name'] ? storage['video_name'][0] : nil
end

def choose_extension
  if audio_device && !video_device
    # TODO 'wav' here once it works with solely wav :)
    storage['current_ext_sans_dot'] = DropDownSelector.new(@frame, ['mp3', 'aac'], "You are set to record only audio--Select audio Save as type").go_selected_value
  else
    storage['current_ext_sans_dot'] = 'mp4' # LODO dry up ".mp4"
  end
end

bootstrap_devices
setup_ui # init the disabled status of the buttons :)
frame.show
elements[:preferences].click! if ARGV[0] == '--options'