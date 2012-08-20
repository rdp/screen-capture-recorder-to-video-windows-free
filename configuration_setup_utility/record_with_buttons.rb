require 'common_recording.rb'
require 'thread'

frame = ParseTemplate.new.parse_setup_filename 'template.record_buttons'
frame.elements[:start].disable
frame.elements[:stop].disable
@frame = frame
storage = Storage.new('record_with_buttons')
@storage = storage
require 'tmpdir'
storage['save_to_dir'] ||= Dir.tmpdir

def current_storage_dir
  @storage['save_to_dir']
end

elements = frame.elements

@elements = elements

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
  ext = @storage['current_ext_sans_dot']
  @next_filename = "#{current_storage_dir}/#{next_number}.#{ext}"
  device_names = [@storage['video_name'], @storage['audio_name']].compact.map{|name| name[0..7]}.join(', ')
  next_file_basename = File.basename(get_old_files[-1] || @next_filename)
  @frame.title = 'To: ' + File.basename(File.dirname(@next_filename)) + '/' + next_file_basename + " from #{device_names}..."
  if(@current_process)
    @elements[:stop].enable 
    @elements[:start].disable
    @elements[:start].text = "Recording!"
  else
    @elements[:stop].disable
    @elements[:start].text = "Start Recording"
    @elements[:start].enable
  end
end

process_input_mutex = Mutex.new

elements[:start].on_clicked {
 if @current_process
   raise 'unexpected double run?'
 else
   if @storage['show_transparent_window_first']
     require 'window_resize'
     @elements[:start].disable
     window = WindowResize.go false
     window.after_closed { start_recording_with_current_settings }
   else
     start_recording_with_current_settings
   end
 end
}

@frame.after_closed {
 if @current_process
   SimpleGuiCreator.show_blocking_message_dialog "an ffmpeg instance was left running, closing it..."
   elements[:stop].simulate_click # just in case :P
 end
}

def start_recording_with_current_settings
   if @storage['video_name']
     codecs = "-vcodec qtrle -acodec ac3"
   else
     codecs = "" # let it auto-select the audio codec based on @next_filename. Weird, I know.
   end
   stop_time = @storage['stop_time']
   if stop_time.present?
     stop_time = "-t #{stop_time}"     
   end

   c = "ffmpeg -threads 1 #{stop_time} #{combine_devices_for_ffmpeg_input @storage['audio_name'], @storage['video_name'] } #{codecs} \"#{@next_filename}\""
   puts "writing to #{@next_filename}"
   puts 'running', c
   @current_process = IO.popen(c, "w") # jruby friendly :P
   if stop_time.present?
     Thread.new { 
       while(@current_process)
         sleep 0.5 
          begin
           process_input_mutex.synchronize { @current_process.puts "a" if @current_process }
         rescue IOError => e
           puts 'process stopped' # one way or the other...
           elements[:stop].simulate_click
         end
       end
     }
   end
   setup_ui
   @frame.title = "Recording to #{File.basename @next_filename}"
end

elements[:stop].on_clicked {
  process_input_mutex.synchronize {
    if @current_process
      # .close might "just kill" ffmpeg, so tell it to shutdown gracfully
      @current_process.puts 'q' rescue nil # can fail, meaning I guess ffmpeg already exited...
      # @current_process.close
      @current_process = nil
      puts # pass the ffmpeg stuff, hopefully
      puts "done writing #{@next_filename}"
      if @storage['reveal_files_after_each_recording'] 
        SimpleGuiCreator.show_in_explorer @next_filename
      end
      setup_ui # re-load the buttons
    else
      # could be they had a timed recording, and clicked stop
    end
  }
}

elements[:preferences].on_clicked {
  audio, video = choose_devices
  storage['video_name'] = video
  storage['audio_name'] = audio
  if audio && !video
    @storage['current_ext_sans_dot'] = DropDownSelector.new(@frame, ['ac3', 'mp3', 'wav'], "Select audio Save as type").go_selected_value
  else
    @storage['current_ext_sans_dot'] = 'mov'
  end
  
  stop_time = SimpleGuiCreator.get_user_input "Automatically stop the recording after a certain number of seconds (leave blank and click ok for it to record till you click the stop button)", @storage[stop_time], true
  @storage['stop_time'] = stop_time
    
  @storage['save_to_dir'] = SimpleGuiCreator.new_existing_dir_chooser_and_go 'select save to dir', current_storage_dir
  
  if SimpleGuiCreator.show_select_buttons_prompt("Would you like to display a resizable setup window before each recording?") == :yes
    @storage['show_transparent_window_first'] = true
  else
    @storage['show_transparent_window_first'] = false
  end

  if SimpleGuiCreator.show_select_buttons_prompt("Would you like to automatically display files after recording them?") == :yes
    @storage['reveal_files_after_each_recording'] = true
  else
    @storage['reveal_files_after_each_recording'] = false
  end
  
  setup_ui
}

if(!storage['video_name'] && !storage['audio_name'])

  need_help = false
  if FfmpegHelpers.enumerate_directshow_devices[:audio].include?(VirtualAudioDeviceName)
    # a reasonable default :P
    storage['audio_name'] = VirtualAudioDeviceName
    @storage['current_ext_sans_dot'] = 'mp3'
  else
    need_help = true
  end
  
  # *my* preferred defaults :P
  if ARGV[0] != '--just-audio-default'
    if FfmpegHelpers.enumerate_directshow_devices[:video].include?(ScreenCapturerDeviceName)
      storage['video_name'] = ScreenCapturerDeviceName
      @storage['current_ext_sans_dot'] = 'mov'	  
    else
      need_help = true
    end
  end  
  elements[:preferences].simulate_click if need_help
  
else
  Thread.new { FfmpegHelpers.warmup_ffmpeg_so_itll_be_disk_cached } # why not? my fake attempt at making ffmpeg realtime startup fast LOL
end

setup_ui # init the disabled status of the buttons :)

frame.show