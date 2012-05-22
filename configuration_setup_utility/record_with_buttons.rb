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

elements['record_to_dir_text'].text += " " + current_storage_dir

elements['reveal_save_to_dir'].on_clicked {
  SwingHelpers.show_in_explorer get_old_files.last
}

elements['stop'].disable # default disable

def get_old_files
  old_files = Dir[current_storage_dir + '/*.{wav,mp4,mp3,mpg}']
  if old_files.empty?
    old_files = [current_storage_dir + "/0.fak"]
	require 'fileutils'
	FileUtils.touch old_files[0] # so 'reveal in explorer' can work
  end
  old_files = old_files.sort_by{|f| f =~ /(\d+)\....$/; $1.to_i}
  old_files
 end
  
numbered = get_old_files.map{ |f| f =~ /(\d+)\....$/; $1.to_i}
next_number = numbered.last + 1

elements['start'].on_clicked {
 if @current_process
   raise 'unexpected'
 else
   if storage['video_name']
     ext = '.mp4'
   else
     ext = '.mp3'
   end
   @next_filename = "#{current_storage_dir}/#{next_number+=1}#{ext}"
   c = "ffmpeg #{combine_devices_for_ffmpeg_input storage['audio_name'], storage['video_name'] } -vcodec huffyuv \"#{@next_filename}\""
   puts 'running', c
   @current_process = IO.popen(c, "w") # jruby friendly :P
   elements['stop'].enable
   elements['start'].disable
 end
}

elements['stop'].on_clicked {
  if @current_process
    @current_process.print 'q' rescue nil # can fail, meaning I guess ffmpeg already exited...
	@current_process.close
	@current_process = nil
    elements['start'].enable
	elements['stop'].disable
	elements['last_file_written'].text= 'wrote file' + File.basename(@next_filename)
	puts # pass the ffmpeg stuff
	puts 'done writing'
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
}

if(!storage['video_name'] && !storage['audio_name'])
  elements['preferences'].simulate_click
end

frame.show