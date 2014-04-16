# coding: UTF-8
require 'common_recording.rb'

def reset_options_frame
  if @options_frame # TODO check this on my desktop what the...
    @options_frame.close
	@options_frame = nil
    # don't show options frame still, it just feels so annoying...
    # elements[:preferences].click!
    SimpleGuiCreator.show_message "Options saved! You're ready to go..."
    setup_ui # reset the main frame too :)
  else
    puts "what, called close twice?"
  end
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
  [Select video device:select_new_video] " #{remove_quotes(video_device_name_or_nil || 'none selected')} :video_name"
  [Select audio devices:select_new_audio] " #{remove_quotes(audio_device_names_or_nil || 'none selected')} :audio_name" 
  [✓:record_to_file] "Save to file"   [ Set file options :options_button]
  [✓:stream_to_url_checkbox] "Stream to url:"  "Specify url first!:url_stream_text" [ Set streaming url : set_stream_url ]
  "Stop recording after this many seconds:" "#{storage['stop_time']}" [ Click to set :stop_time_button]
  "Current record resolution: #{resolution_english_string storage['resolution']} :fake" [Change :change_resolution]
  "Current record video fps: #{storage['fps'] || default_fps_string} :fake2" [Change :change_fps]
  [Preview current settings:preview] "a rough preview of how the recording will look"
  [ Close Options Window :close]
  EOL
  # print template
  # TODO it can automatically 'bind' to a storage, and automatically 'always call this method for any element after clicked' :)
  
  @options_frame = ParseTemplate.new.parse_setup_string template
  frame = @options_frame
  if storage['should_record_to_file']
    frame.elements[:record_to_file].set_checked!
  else
    frame.elements[:record_to_file].set_unchecked!
  end
  frame.elements[:record_to_file].on_clicked { |new_value|
    if @options_frame
      storage['should_record_to_file'] = new_value
	  reset_options_frame
	end
  }
  
  if storage['should_stream']
    frame.elements[:stream_to_url_checkbox].check!
  else
    frame.elements[:stream_to_url_checkbox].uncheck!
  end
  frame.elements[:stream_to_url_checkbox].on_clicked { |new_value|
    if @options_frame # XXX rdp what the..>?
      storage['should_stream'] = new_value
      reset_options_frame
	end
  }
  
  if !storage[:url_stream].present?
    frame.elements[:stream_to_url_checkbox].set_unchecked!
    frame.elements[:stream_to_url_checkbox].disable! # can't check it if there's nothing to use...
  else
    frame.elements[:url_stream_text].text = shorten(storage[:url_stream], 20)
  end
  
  frame.elements[:set_stream_url].on_clicked {
    stream_url = SimpleGuiCreator.get_user_input "Url to stream to, like udp://236.0.0.1:2000", storage[:url_stream], true
    storage[:url_stream] = stream_url
	# TODO audio for receipt latency?
	puts "receive it like mplayer demuxer +mpegts -framedrop -benchmark ffmpeg://udp://236.0.0.1:2000?fifo_size=1000000&buffer_size=1000000"
	reset_options_frame
  }
  
  frame.elements[:preview].on_clicked {
    start_recording_with_current_settings true
  }
  
  frame.elements[:select_new_video].on_clicked {
    choose_video
  }
  
  frame.elements[:select_new_audio].on_clicked {
    choose_audio
  }
  
  raise unless RUBY_VERSION > '1.9.0' # need ordered hashes here... :)
  frame.elements[:change_resolution].on_clicked {
    # want to have our own names here, as requested...  
	english_names = ResolutionOptions.map{|k, v| resolution_english_string(v)}
    idx = DropDownSelector.new(nil, english_names, "Select new resolution").go_selected_index
	resolution = ResolutionOptions.to_a[idx][1] # get the value...hmm...
	storage['resolution'] =  resolution
	puts 'set it to', resolution
	reset_options_frame
  }
  
  frame.elements[:change_fps].on_clicked {
    options = [default_fps_string] + (5..30).step(5).to_a   # TODO didn't I have code somewhere that enumerated this on the fly? That would be better...
    fps = DropDownSelector.new(nil, options, "Select new video fps").go_selected_value
    if fps == default_fps_string
	  storage['fps'] = nil
	else
	  storage['fps'] = fps
	end
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
  reset_options_frame
end

def choose_audio
  all_audio_devices = FFmpegHelpers.enumerate_directshow_devices[:audio]
  template = "----Audio Device choice---
  Select which audio devices to record, if any:"
  audio_pane = ParseTemplate.new.parse_setup_string template # shows by default
  # sanity check that some haven't disappeared
  audio_devices.each{|currently_checked|
    if !all_audio_devices.contain?(currently_checked)
	  SimpleGuiCreator.show_blocking_message_dialog "warning: removed now unfound audio device #{currently_checked[0]}"
	  storage['audio_names'].delete(currently_checked)
	  storage.save!
	end
  }
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
	if(audio_devices.include? [audio_device_name, audio_device_idx])
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

def choose_extension
  if audio_devices && !video_device
    # TODO 'wav' here once it works with solely wav :)
    storage['current_ext_sans_dot'] = DropDownSelector.new(@frame, ['mp3', 'aac'], "You are set to record only audio--Select audio Save as type").go_selected_value
  else
    storage['current_ext_sans_dot'] = 'mp4' # LODO dry up ".mp4"
  end
end
