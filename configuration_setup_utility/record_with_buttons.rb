# coding: UTF-8
require 'common_recording.rb'
require 'thread'

frame = ParseTemplate.new.parse_setup_filename 'template.record_buttons'
frame.elements[:start].disable
frame.elements[:stop].disable
@frame = frame

@storage = Storage.new('record_with_buttons')
def storage
  @storage
end

require 'tmpdir'
storage['save_to_dir'] ||= Dir.tmpdir

storage.set_default('record_to_file', true)

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

def setup_ui
  numbered = get_old_files.map{ |f| f =~ /(\d+)\....$/; $1.to_i}
  next_number = (numbered[-1] || 0) + 1
  ext = storage['current_ext_sans_dot']
  @next_filename = "#{current_storage_dir}/#{next_number}.#{ext}"
  device_names = [storage['video_name'], storage['audio_name']].compact.map{|name| name[0..7]}.join(', ')
  next_file_basename = File.basename(get_old_files[-1] || @next_filename)
  @frame.title = 'To: ' + File.basename(File.dirname(@next_filename)) + '/' + next_file_basename + " from #{device_names}..."
  if(@current_process)
    elements[:stop].enable 
    elements[:start].disable
    elements[:start].text = "Recording!"
  else
    elements[:stop].disable
    elements[:start].text = "Start Recording"
    elements[:start].enable
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
     window = WindowResize.go false
     window.after_closed { start_recording_with_current_settings }
   else
     start_recording_with_current_settings
   end
 end
}

@frame.after_closed {
 if @current_process
   SimpleGuiCreator.show_blocking_message_dialog "an ffmpeg instance was left running, will close it for you..."
   elements[:stop].simulate_click # just in case :P
 end
}

def start_recording_with_current_settings

   unless storage['video_name'] || storage['audio_name']
     SimpleGuiCreator.show_blocking_message_dialog('must select at least one')
     return
   end

   if storage['video_name']
     codecs = "-vcodec libx264 -preset ultrafast -acodec ac3" # qtrle was 10 fps, this kept up at 15 on dual core, plus is .mp4 friendly, though lossy, looked all right
   else
     codecs = "" # let it auto-select the audio codec based on @next_filename. Weird, I know.
   end
   stop_time = storage['stop_time']
   if stop_time.present?
     stop_time = "-t #{stop_time}"     
   end

   c = "ffmpeg #{stop_time} #{combine_devices_for_ffmpeg_input storage['audio_name'], storage['video_name'] } #{codecs} \"#{@next_filename}\""
   puts "writing to #{@next_filename}"
   puts 'running', c
   @current_process = IO.popen(c, "w") # jruby friendly :P
   Thread.new { 
     # handle early outs...
     wait_until_current_process_exits
	 elements[:stop].simulate_click
   }
   setup_ui
   @frame.title = "Recording to #{File.basename @next_filename}"
end

def wait_until_current_process_exits
       still_alive = true
       while still_alive
         sleep 0.2 
         begin
		   if !@current_process # this smells funny...
		     still_alive = false
		   else
             Process.kill Signal.list['EXIT'], @current_process.pid
		   end
         rescue Errno::EPERM => e
           puts 'process stopped' # one way or the other...          
		   still_alive = false
         end
       end
end

elements[:stop].on_clicked {
  process_input_mutex.synchronize {
    if @current_process
      # .close might "just kill" ffmpeg, and skip trailers in certain muxes, so tell it to shutdown gracfully
      @current_process.puts 'q' rescue nil # can fail, meaning I guess ffmpeg already exited...
	  wait_until_current_process_exits	  
	  @current_process = nil
      # @current_process.close
      puts # pass the ffmpeg stuff, hopefully
      puts "done writing #{@next_filename}"
      if storage['reveal_files_after_each_recording'] 
        SimpleGuiCreator.show_in_explorer @next_filename
      end
      setup_ui # re-load the buttons
    else
      # could be they had a timed recording, and clicked stop
    end
  }
}

def reset_options_frame
  @options_frame.close
  setup_ui
  elements[:preferences].click!
end

elements[:preferences].on_clicked {
  template = <<-EOL
  ------------ Recording Options -------------
  " #{video_device || 'none selected'} :video_name" [Select video device:select_new_video]
  " #{audio_device || 'none selected'} :audio_name" [Select audio device:select_new_audio]
  "Stop recording after this many seconds:" "#{storage['stop_time']}" [ Click to set :stop_time_button]
  "Save to file:" [✓:record_to_file]
  "Stream to url:" [✓:stream_to_url_checkbox] 
  "#{storage[:url_stream] || '      none'}:width=250" [ Set streaming url : set_stream_url ]
  [ Set options (directories, etc.) :options_button]
  [ Close :close]
  EOL
  
  @options_frame = ParseTemplate.new.parse_setup_string template
  frame = @options_frame
  if storage['record_to_file']
    frame.elements[:record_to_file].set_checked!
  end
  frame.elements[:record_to_file].on_clicked { |new_value|
    storage['record_to_file'] = new_value
	reset_options_frame
  }
  if storage['should_stream_to_url']
    frame.elements[:stream_to_url_checkbox].set_checked!
  end
  frame.elements[:stream_to_url_checkbox].on_clicked {|new_value|
    storage['should_stream_to_url'] = new_value
    reset_options_frame
  }
  
  frame.elements[:set_stream_url].on_clicked {
    stream_url = SimpleGuiCreator.get_user_input "Url to stream to, like rtmp://live.justin.tv/app/live_XXXXXX", storage[:url_stream], true
    storage[:url_stream] = stream_url
	reset_options_frame
  }
  
  frame.elements[:select_new_video].on_clicked {
    choose_video
	reset_options_frame
  }  
  frame.elements[:select_new_audio].on_clicked {
    choose_audio
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
  
    if storage['video_name'] # don't prompt if they're audio only
      if SimpleGuiCreator.show_select_buttons_prompt("Would you like to display a resizable setup window before each recording?") == :yes
        storage['show_transparent_window_first'] = true
      else
        storage['show_transparent_window_first'] = false
      end
    end

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
  if FfmpegHelpers.enumerate_directshow_devices[:audio].include?(VirtualAudioDeviceName)
    # a reasonable default :P
    storage['audio_name'] = VirtualAudioDeviceName
    storage['current_ext_sans_dot'] = 'mp3'
  else
    need_help = true
  end
  
  # *my* preferred defaults :P
  if ARGV[0] != '--just-audio-default'
    if FfmpegHelpers.enumerate_directshow_devices[:video].include?(ScreenCapturerDeviceName)
      storage['video_name'] = ScreenCapturerDeviceName
      storage['current_ext_sans_dot'] = 'mp4'  
    else
      need_help = true
    end
  end  
  elements[:preferences].simulate_click if need_help
  
else
  Thread.new { FfmpegHelpers.warmup_ffmpeg_so_itll_be_disk_cached } # why not? my fake attempt at making ffmpeg realtime startup fast LOL
end

end

def choose_video
  videos = ['none'] + FfmpegHelpers.enumerate_directshow_devices[:video].sort_by{|name| name == ScreenCapturerDeviceName ? 0 : 1}  # put none in front :)
  original_video_device = video_device 
  video_device = DropDownSelector.new(nil, videos, "Select video device to capture, or \"none\" to record audio only").go_selected_value
  if video_device == 'none'
    video_device = nil
  else
    # stay same
  end
  storage['video_name'] = video_device
  
  if video_device == ScreenCapturerDeviceName
    SimpleGuiCreator.show_blocking_message_dialog "you can setup parameters [like frames per second, size] for the screen capture recorder\n in its separate setup configuration utility"
  end
  choose_extension
end

def choose_audio  
  audios = ['none'] + FfmpegHelpers.enumerate_directshow_devices[:audio].sort_by{|name| name == VirtualAudioDeviceName ? 0 : 1}
  audio_device = DropDownSelector.new(nil, audios, "Select audio device to capture, or none").go_selected_value
  if audio_device == 'none'
    audio_device = nil 
  else
    # stay same
  end
  storage['audio_name'] = audio_device
  choose_extension
end

def audio_device 
  storage['audio_name']
end

def video_device 
  storage['video_name']
end

def choose_extension
  if audio_device && !video_device
    storage['current_ext_sans_dot'] = DropDownSelector.new(@frame, ['ac3', 'mp3', 'wav'], "You are set to record only audio--Select audio Save as type").go_selected_value
  else
    storage['current_ext_sans_dot'] = 'mp4' # LODO dry up ".mp4"
  end
end

bootstrap_devices
setup_ui # init the disabled status of the buttons :)
frame.show