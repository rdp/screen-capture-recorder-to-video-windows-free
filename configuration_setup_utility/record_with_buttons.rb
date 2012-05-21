require 'common_recording.rb'
require 'jruby-swing-helpers/parse_template.rb'
require 'jruby-swing-helpers/storage.rb'

frame = ParseTemplate.parse_file 'record_buttons.template'
storage = Storage.new('record_with_buttons')
require 'tmpdir'
storage['save_to_dir'] ||= Dir.tmpdir

elements = frame.elements

elements['record_to_dir_text'].text += " " + storage['save_to_dir']

elements['reveal_save_to_dir'].on_clicked {
  SwingHelpers.show_in_explorer storage['save_to_dir']
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