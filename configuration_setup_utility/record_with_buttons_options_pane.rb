# coding: UTF-8
require 'common_recording.rb'

def reset_options_frame  
  @options_frame.close
  show_options_frame # show a new one--it's just barely too jarring otherwise, even with a single checkbox, it just disappears, and we have useful information in that options window now...
  # show_message "Options saved! You're ready to go..."
  if should_save_file? && should_stream?
    show_message "warning, yours is set to both save to file *and* stream which isn't supported yet, ping me to have it added\n[for now, please uncheck one of the file or stream checkboxes]!"	  
  end
  setup_ui # reset the main frame too, why not, though we don't have to...
end

def remove_quotes string
  string.gsub('"', '')
end

ResolutionOptions = {'native input (unscaled)' => nil, 'vga' => '640x480', 'svga' => '800x600', 'xga' => '1024x768',
'PAL' => '768x576', 'hd720' => '1280x720', 'hd1080' => '1980x1080'}	

def resolution_english_string resolution
  if resolution
    ResolutionOptions.key(resolution) + " (" + resolution + ")"
  else
    ResolutionOptions.key(resolution) # nothing to add on the end ...
  end
end

def default_fps_string
  'native fps'
end

def show_options_frame
  template = <<-EOL
  ------------ Recording Options -------------
  [Close Options Window :close]
  [Select video device:select_new_video] " #{remove_quotes(video_device_name_or_nil || 'none selected')} :video_name"
  EOL
  
  if video_device_name_or_nil == ScreenCapturerDeviceName
    template += %!"Screen Capture Recorder specific options:"\n!
    template += %!"            " [Configure screen recorder bounds box:configure_screen_capturer_window]\n!
    template += %!"            " "Advanced:" [Configure screen recorder by numbers:configure_screen_capturer_numbers]\n!
	template += %!"            " [✓:show_resize_window_each_time_button] "Display select window before every recording."\n!
	
  end
  
  template += <<-EOL  
  [Select audio devices:select_new_audio] " #{remove_quotes(audio_device_names_or_nil || 'none selected')} :audio_name" 
  [✓:record_to_file] "Save to file"  "an awesome file location!!!!!:save_to_dir_text" [ Set directory :set_directory]
  [✓:auto_display_files] "Automatically reveal files in windows explorer after each recording:auto_display_files_text"
  [✓:stream_to_url_checkbox] "Stream to url:"  "Specify url first!!!!!!!!!:url_stream_text" [ Set streaming options : set_stream_url ]
  [✓:tune_latency] "Tune for low latency"
  "Stop recording after this many seconds:" "#{storage['stop_time']}" [ Click to set :stop_time_button]
  "Current scale-to resolution: #{resolution_english_string storage['resolution']} :fake" [Change :change_resolution]
  "Current video input fps: #{storage['fps'] || default_fps_string} :fake2" [Change :change_fps]
  [Preview current settings:preview] "a rough preview of how the recording will look"
  [Close Options Window :close2]
  EOL
  # print template
  # TODO it can automatically 'bind' to a storage, and automatically 'always call this method for any element after clicked' :)
  
  @options_frame = ParseTemplate.new.parse_setup_string template
  frame = @options_frame
  elements = frame.elements
  
  if video_device_name_or_nil == ScreenCapturerDeviceName
    elements[:configure_screen_capturer_numbers].on_clicked {
      require './setup_via_numbers'
	  do_setup_via_numbers
    }
	elements[:configure_screen_capturer_window].on_clicked {
	  window = WindowResize.go false, false
      window.after_closed { show_message('ok, future recordings will be from there, to reset it to full screen, run setup via numbers and clear the values for height and width') }
	}
    elements[:show_resize_window_each_time_button].set_checked!( storage['show_transparent_window_first'] )
	elements[:show_resize_window_each_time_button].on_clicked { |new_value|
	  storage['show_transparent_window_first'] = new_value 
	}
  end
  
  if storage['should_record_to_file']
    elements[:record_to_file].set_checked!
  else
    elements[:record_to_file].set_unchecked!
	elements[:save_to_dir_text].disable! # grey it out
	elements[:auto_display_files_text].disable! # same
  end
  elements[:record_to_file].on_clicked { |new_value|
    storage['should_record_to_file'] = new_value
	reset_options_frame
  }
  # doesn't work? storage['reveal_files_after_each_recording'].present?
  elements[:auto_display_files].set_checked!( storage['reveal_files_after_each_recording'] )
  elements[:auto_display_files].on_clicked { |new_value|
    storage['reveal_files_after_each_recording'] = new_value
	reset_options_frame
  }
  
  elements[:save_to_dir_text].text = shorten(storage['save_to_dir'], 20) # assume there's always one...	
  elements[:set_directory].on_clicked {  
    storage['save_to_dir'] = SimpleGuiCreator.new_existing_dir_chooser_and_go 'select save to dir', current_storage_dir
	reset_options_frame
  }  
  
  if storage['should_stream']
    elements[:stream_to_url_checkbox].check!
  else
    elements[:stream_to_url_checkbox].uncheck!
	elements[:url_stream_text].disable! # grey it out
  end
  elements[:stream_to_url_checkbox].on_clicked { |new_value|
    storage['should_stream'] = new_value	  
    reset_options_frame
  }
  
  if !storage[:url_stream].present?
    elements[:stream_to_url_checkbox].uncheck!
    elements[:stream_to_url_checkbox].disable! # can't check it if there's nothing to use...	
  else
    elements[:url_stream_text].text = shorten(storage[:url_stream], 20)	
  end
  if storage[:tune_latency]
    elements[:tune_latency].check!
  else
    elements[:tune_latency].uncheck!
  end
  elements[:tune_latency].on_clicked { |new_value|
    storage[:tune_latency] = new_value	  
	reset_options_frame # LODO never have a new one show up :)
  }
  
  elements[:set_stream_url].on_clicked {
    stream_url = SimpleGuiCreator.get_user_input "Url to stream to, like udp://236.0.0.1:2000 (ping me if you want rtmp added)\nreceive it like \nmplayer -demuxer +mpegts -framedrop -benchmark ffmpeg://udp://236.0.0.1:2000?fifo_size=1000000&buffer_size=1000000\nwith patched mplayer", storage[:url_stream], false
    storage[:url_stream] = stream_url
	# TODO audio for receipt latency?
	reset_options_frame
  }
  
  elements[:preview].on_clicked {
    start_recording_with_current_settings true
  }
  
  elements[:select_new_video].on_clicked {
    choose_video
  }
  
  elements[:select_new_audio].on_clicked {
    choose_audio
  }
  
  raise unless RUBY_VERSION > '1.9.0' # need ordered hashes here... :)
  elements[:change_resolution].on_clicked {
    # want to have our own names here, as requested...  
	english_names = ResolutionOptions.map{|k, v| resolution_english_string(v)}
    idx = DropDownSelector.new(nil, english_names, "Select new resolution").go_selected_index
	resolution = ResolutionOptions.to_a[idx][1] # get the value...hmm...
	storage['resolution'] =  resolution
	puts 'set it to', resolution
	reset_options_frame
  }
  
  elements[:change_fps].on_clicked {
    options = [default_fps_string] + (5..30).step(5).to_a   # TODO didn't I have code somewhere that enumerated this on the fly? That would be better...
    fps = DropDownSelector.new(nil, options, "Select new video fps").go_selected_value
    if fps == default_fps_string
	  storage['fps'] = nil
	else
	  storage['fps'] = fps
	end
	reset_options_frame
  }
  
  close_proc = proc { 
    frame.close
    setup_ui # reset parent	
  }
  elements[:close].on_clicked &close_proc
  elements[:close2].on_clicked &close_proc

  elements[:stop_time_button].on_clicked {  
    stop_time = SimpleGuiCreator.get_user_input "Automatically stop the recording after a certain number of seconds (leave blank and click ok for it to record till you click the stop button)", storage['stop_time'], true
    storage['stop_time'] = stop_time
	reset_options_frame
  }

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
  
  choose_extension
  reset_options_frame
end

def choose_audio
  all_audio_devices = FFmpegHelpers.enumerate_directshow_devices[:audio]
  template = "----Audio Device choice---
  Select which audio devices to record, if any:"
  audio_pane = ParseTemplate.new.parse_setup_string template # shows by default
  all_audio_devices.each_with_index{|(audio_device_name, audio_device_idx), idx|
    audio_device = [audio_device_name, audio_device_idx]
    button_name = :"choose_audio_#{idx}"
    next_line =  "[✓:#{button_name}] \"#{audio_device_name} #{audio_device_idx if audio_device_idx > 1}\""
	p next_line
	audio_pane.add_setup_string_at_bottom next_line
	checkbox = audio_pane.elements[button_name]
	checkbox.after_checked {
	  storage['audio_names'] << audio_device # add it
	  storage.save!
	  puts "now #{audio_device_names_or_nil}"
	}
	checkbox.after_unchecked {
	  storage['audio_names'].delete audio_device
	  storage.save!
	  puts "now #{audio_device_names_or_nil}"
	}	
	if((audio_devices_or_nil || []).include? [audio_device_name, audio_device_idx])
	  checkbox.check!
	else
	  checkbox.uncheck!
	end	  
  }
  audio_pane.add_setup_string_at_bottom "[Done selecting audio:done_selecting]" 
  audio_pane.elements[:done_selecting].on_clicked {
    audio_pane.close
  }
  audio_pane.after_closed {
    choose_extension
    reset_options_frame
  }
end
