require 'common_recording.rb'
require 'jruby-swing-helpers/parse_template.rb'
require 'jruby-swing-helpers/storage.rb'

frame = ParseTemplate.parse_file 'record_buttons.template'
storage = Storage.new('record_with_buttons')
@storage = storage
require 'tmpdir'
storage['save_to_dir'] ||= Dir.tmpdir

elements = frame.elements

elements['record_to_dir_text'].text += " " + current_storage_dir

elements['reveal_save_to_dir'].on_clicked {
  SwingHelpers.show_in_explorer current_storage_dir
}

elements['stop'].disable

old_number = Dir[current_storage_dir + '/*.{wav,mp4}'].map{ |f| f =~ /(\d+)/; $1.to_i}.sort.last + 1

def current_storage_dir
  @storage['save_to_dir']
end

elements['start'].on_clicked {
 if @current_process
   raise 'unexpected'
 else
   # TODO ffmpeg
   if storage['video_name']
     ext = '.mp4'
   else
     ext = '.mp3'
   end
   "ffmpeg -f dshow  -vcodec huffyuv #{current_storage_dir}/#{old_number+=1}.#{ext}"
   @current_process = open("|", "w")
   elements['stop'].enable
 end
}

elements['stop'].on_clicked {
  if @current_process
    @current_process.print 'q'
	@current_process.close
	@current_process = nil
    elements['start'].enable
  else
    raise 'unexpected2'
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
}

if(!storage['video_name'] && !storage['audio_name'])
  elements['preferences'].simulate_click
end

frame.show