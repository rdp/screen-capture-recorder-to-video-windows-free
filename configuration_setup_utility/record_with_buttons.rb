require 'common_recording.rb'

require 'jruby-swing-helpers/lib/parse_template.rb'
require 'jruby-swing-helpers/lib/storage.rb'

frame = ParseTemplate.parse_file 'record_buttons.template'
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

elements['reveal_save_to_dir'].on_clicked {
  last_filename = get_old_files.last
  if last_filename
    SwingHelpers.show_in_explorer last_filename
  else
	SwingHelpers.show_blocking_message_dialog "none have been recorded yet, so revealing the directory they will be recorded to"
    SwingHelpers.show_in_explorer current_storage_dir	
  end
}

def get_old_files
  old_files = Dir[current_storage_dir + '/*.{wav,mp4,mkv,mp3,mpg}']  
  old_files.select!{|f| File.basename(f) =~ /^\d+\..../}
  old_files = old_files.sort_by{|f| f =~ /(\d+)\....$/; $1.to_i}
  old_files
end

def setup_ui
  numbered = get_old_files.map{ |f| f =~ /(\d+)\....$/; $1.to_i}
  next_number = (numbered[-1] || 0) + 1
  ext = @storage['current_ext_sans_dot']
  @next_filename = "#{current_storage_dir}/#{next_number}.#{ext}"
  device_names = [@storage['video_name'], @storage['audio_name']].compact.join(',')
  next_file_basename = File.basename(get_old_files[-1] || @next_filename)
  @frame.title = 'Recorded to: ' + File.basename(File.dirname(@next_filename)) + '/' + next_file_basename + " #{device_names}"
  if(@current_process)
    @elements['stop'].enable 
	@elements['start'].disable
	@elements['start'].text = "Recording!"
  else
    @elements['stop'].disable
	@elements['start'].enable
	@elements['start'].text = "Start Recording"
  end
end

elements['start'].on_clicked {
 if @current_process
   raise 'unexpected'
 else
   if storage['video_name']
     codecs = "-vcodec huffyuv -acodec ac3"
   else
     codecs = "" # let it auto-select the audio codec
   end
   c = "ffmpeg #{combine_devices_for_ffmpeg_input storage['audio_name'], storage['video_name'] } #{codecs} \"#{@next_filename}\""
   puts 'running', c
   @current_process = IO.popen(c, "w") # jruby friendly :P
   setup_ui
 end
}

elements['stop'].on_clicked {
  if @current_process
    @current_process.print 'q' rescue nil # can fail, meaning I guess ffmpeg already exited...
	@current_process.close
	@current_process = nil
	puts # pass the ffmpeg stuff, hopefully
	puts 'done writing'
	setup_ui
  else
    raise 'unexpected2?'
  end
}

elements['preferences'].on_clicked {
  audio, video = choose_devices
  storage['video_name'] = video
  storage['audio_name'] = audio
  if audio && !video
    @storage['current_ext_sans_dot'] = DropDownSelector.new(@frame, ['ac3', 'mp3', 'wav'], "Select audio Save as type").go_selected_value
  else
    @storage['current_ext_sans_dot'] = 'mkv'
  end
    
  @storage['save_to_dir'] = SwingHelpers.new_existing_dir_chooser_and_go 'select save to dir', current_storage_dir
  setup_ui
}

if(!storage['video_name'] && !storage['audio_name'])
  if ARGV[0] == '--just-audio-default' && FfmpegHelpers.enumerate_directshow_devices[:audio].include?(VirtualAudioDeviceName)
    storage['audio_name'] = VirtualAudioDeviceName
	@storage['current_ext_sans_dot'] = 'mp3'
  else
    elements['preferences'].simulate_click # setup preferences once
  end 
else
  Thread.new { FfmpegHelpers.warmup_ffmpeg_so_itll_be_disk_cached } # why not? my fake attempt at making ffmpeg realtime friendly LOL
end

setup_ui # init :)

frame.show