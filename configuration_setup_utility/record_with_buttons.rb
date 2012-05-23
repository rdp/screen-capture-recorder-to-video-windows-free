require 'common_recording.rb'

require 'jruby-swing-helpers/parse_template.rb'
require 'jruby-swing-helpers/storage.rb'

frame = ParseTemplate.parse_file 'record_buttons.template'

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
  old_files = Dir[current_storage_dir + '/*.{wav,mp4,mp3,mpg}']  
  old_files = old_files.sort_by{|f| f =~ /(\d+)\....$/; $1.to_i}
  old_files
end

def setup_ui
  numbered = get_old_files.map{ |f| f =~ /(\d+)\....$/; $1.to_i}
  @next_number = (numbered.last || 0) + 1
  if @storage['video_name']
   ext = '.mp4'
  else
   ext = '.mp3'
  end
  @next_filename = "#{current_storage_dir}/#{@next_number}#{ext}"
  @elements['next_file_written'].text= 'Next file will write to: ' + File.basename(@next_filename) # TODO .original_text :P
  @elements['record_to_dir_text'].text = "Record To Dir currently: " + current_storage_dir
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

setup_ui

elements['start'].on_clicked {
 if @current_process
   raise 'unexpected'
 else
   c = "ffmpeg #{combine_devices_for_ffmpeg_input storage['audio_name'], storage['video_name'] } -vcodec huffyuv \"#{@next_filename}\""
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

def setup_device_text elements, storage
  for type in ['video_name', 'audio_name']
    if storage[type]
	  elements[type].text = "Current device: #{storage[type]}"
	else
	  elements[type].text = ""
	end
  end
end

setup_device_text elements, storage

elements['preferences'].on_clicked {
  audio, video = choose_devices
  storage['video_name'] = video
  storage['audio_name'] = audio
  setup_device_text elements, storage
  @storage['save_to_dir'] = SwingHelpers.new_existing_dir_chooser_and_go 'select save to dir', current_storage_dir
  setup_ui
}

if(!storage['video_name'] && !storage['audio_name'])
  if ARGV[0] == '--just-audio-default' && FfmpegHelpers.enumerate_directshow_devices[:audio].include?(VirtualAudioDeviceName)
    storage['audio_name'] = VirtualAudioDeviceName
  else
    elements['preferences'].simulate_click # setup preferences once
  end 
end

setup_ui
frame.show