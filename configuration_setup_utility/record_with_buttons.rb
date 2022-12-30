require 'common_recording.rb'
require 'thread'

frame = ParseTemplate.new.parse_setup_filename 'template.record_buttons'
frame.elements[:start].disable
frame.elements[:stop].disable
@frame = frame

inno_setup_for_version = File.read('../innosetup_installer_options.iss')
inno_setup_for_version =~ /AppVer "(.*)"/
VERSION = $1
puts "starting version #{VERSION}"

@storage = Storage.new('record_with_buttons_v7') # bump this when stored settings format changes :)
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
    show_message "none have been recorded yet, so revealing the directory they will be recorded to"
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

def shorten string, length_desired=7
  if string && string.size > 0
    if(string.size > length_desired)
      string[0..length_desired] + '...'
	else
	  string
	end
  else
    ""
  end
end

def get_title_suffix
  device_names = [video_device_name_or_nil, audio_device_names_or_nil].compact
  if device_names.length == 2 # both together is too big, truncate
    orig_names = device_names
    device_names = device_names.map{|name| shorten(name)}.join(', ')
  else
    # leave as full single device name...
	device_names = device_names[0]
  end
  title = ''
  destinos = []
  next_file_basename = File.basename(@next_filename)
  if should_save_file?
    destinos << File.basename(File.dirname(@next_filename)) + '/' + next_file_basename
  end
  if should_stream?
    destinos << shorten(storage[:url_stream], 10)
  end
  title += destinos.join(', ')
  title += " from #{device_names} (v.#{VERSION})" # may as well allow them to expand the window and see the version too :)
  title
end

def setup_ui
  numbered = get_old_files.map{ |f| f =~ /(\d+)\....$/; $1.to_i}
  next_number = (numbered[-1] || 0) + 1
  ext = storage['current_ext_sans_dot']
  @next_filename = "#{current_storage_dir}/#{next_number}.#{ext}"
  
  @frame.title = 'Will record to: ' + get_title_suffix
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

require 'window_resize'

elements[:start].on_clicked {
 if @current_process
   raise 'unexpected double start?'
 else
   if video_device && storage['show_transparent_window_first'] # don't display window if not recording video
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
   show_message "an ffmpeg instance was left running, will close it for you..."
   elements[:stop].click! # just in case :P
 end
}

def start_recording_with_current_settings just_preview = false

   unless video_device || audio_devices_or_nil
     show_message('must select at least one of video or audio') # just in case...
     return
   end
   
   if storage['resolution'].present?
     rescale_to_size = "-s #{storage['resolution']}" # ffmpeg rescale, not input scale selector [XXX]
   end
   
   if storage['video_name']
     pixel_type = "yuv420p" # more compatible...?
     # pixel_type = "yuv444p" # didn't help that one guy...but might be good nonetheless
	 # just assume mp3 LOL
	 if storage[:tune_latency]
	   faster_streaming_start = "-g 30 -qp 10 -tune zerolatency" # also has fixed quality...wonder what that means...
	 end
     codecs = "-vcodec libx264 #{faster_streaming_start} -pix_fmt #{pixel_type} #{rescale_to_size} -preset ultrafast -vsync vfr -acodec libmp3lame" 
	 # qtrle was 10 fps, this kept up at 15 on dual core, plus is .mp4 friendly, though lossy, looked all right
   else
     prefix_to_audio_codec = {'mp3' => '-acodec libmp3lame -ac 2', 'aac' => '-acodec aac -strict experimental', 'wav' => '-acodec pcm_s16le'}
     audio_type = prefix_to_audio_codec[storage['current_ext_sans_dot']]
	 raise 'unknown prefix?' + storage['current_ext_sans_dot'] unless audio_type
     codecs = audio_type
   end
   
   if storage['stop_time'].present?
     stop_time = "-t #{storage['stop_time']}"     
   end  
   
   ffmpeg_input = FFmpegHelpers.combine_devices_for_ffmpeg_input audio_devices_or_nil, video_device, storage['fps']
   ffmpeg_commands = "#{ffmpeg_input} #{codecs}"
   if just_preview
     # doesn't take audio, lame...
	 if !storage['video_name']
	    show_message 'you only have audio, this button only previews video for now, ping me to have it improved...' # just in case...
        return
	 end
	 # lessen volume in case of feedback during preview...
	 # analyzeduration 0 to make it pop up quicker...
     c = "ffmpeg #{ffmpeg_commands} -af volume=0.75 -f flv - | ffplay -analyzeduration 0 -f flv -" # couldn't get multiple dshow, -f flv to work... lodo try ffplay with lavfi...maybe?
	 puts c
	 system c
	 puts 'done preview' # clear screen...
	 return
   end
   
   # panic here causes us to not see useful x264 error messages...
   c = "ffmpeg -loglevel info #{stop_time} #{ffmpeg_commands} "
   
   if should_save_file? && should_stream?
	  show_message "warning, yours is set to both save to file *and* stream which isn't supported yet, ping me to have it added!"	  
	  return
   end

   if should_save_file?
	 c += " -f matroska \"#{@next_filename}\""
     puts "writing to file: #{@next_filename}"
   end
   
   if should_stream?
     c += " -f mpegts #{storage[:url_stream]}"
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
   @frame.title = "Recording to " + get_title_suffix
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

elements[:preferences].on_clicked {
  require './record_with_buttons_options_pane'
  show_options_frame
}

def audio_devices_or_nil
  devices = storage['audio_names']
  if(!devices || devices.size == 0)
    nil
  else
    devices
  end
end

def video_device 
  storage['video_name']
end

def audio_device_names_or_nil
  if audio_devices_or_nil
    audio_devices_or_nil.map{|device_name, idx| device_name}.join(', ')
  else
    nil
  end
end

def video_device_name_or_nil
  storage['video_name'] ? storage['video_name'][0] : nil
end


def bootstrap_devices
	if(!video_device && !audio_devices_or_nil)
	  need_help = false
	  # always do the audio..
	  if FFmpegHelpers.enumerate_directshow_devices[:audio].include?([VirtualAudioDeviceName, 0])
		# some reasonable defaults :P
		storage['audio_names'] = [[VirtualAudioDeviceName, 0]]
		storage['current_ext_sans_dot'] = 'mp3'
	  else
		need_help = true
	  end
	  
	  # *my* preferred defaults for combined video/audio :)
	  if ARGV[0] != '--just-audio-default'
		if FFmpegHelpers.enumerate_directshow_devices[:video].include?([ScreenCapturerDeviceName, 0])
		  storage['video_name'] = [ScreenCapturerDeviceName, 0]
		  storage['current_ext_sans_dot'] = 'mkv'  
		else
		  need_help = true
		end
	  end
	  if need_help
	    elements[:preferences].click!
	  end
	else
	  FFmpegHelpers.warmup_ffmpeg_so_itll_be_disk_cached # why not? my fake attempt at making ffmpeg realtime startup fast LOL
	end
	# sanity check that some audio haven't disappeared/unplugged
    all_audio_devices = FFmpegHelpers.enumerate_directshow_devices[:audio]
    (audio_devices_or_nil || []).each{|currently_checked|
      if !all_audio_devices.contain?(currently_checked)
	    SimpleGuiCreator.show_blocking_message_dialog "warning: removed now unfound audio device #{currently_checked[0]}"
	    storage['audio_names'].delete(currently_checked)
	    storage.save!
	  end
    }
	# and video...
	if storage['video_name']
	  all_video_devices = FFmpegHelpers.enumerate_directshow_devices[:video]
	  if !all_video_devices.contain? storage['video_name']
	    SimpleGuiCreator.show_blocking_message_dialog "warning: removed now unfound video device #{storage['video_name'][0]}"
	    storage['video_name'] = nil
	  end
  	  choose_extension # in case we are audio only now <sigh>
	end
end


def choose_extension
  if audio_devices_or_nil && !video_device
    # TODO 'wav' here once it works with solely wav :)
    storage['current_ext_sans_dot'] = DropDownSelector.new(@frame, ['mp3', 'aac'], "You are set to record only audio--Select audio Save as type").go_selected_value
  else
    storage['current_ext_sans_dot'] = 'mkv' # LODO dry up ".mp4"
  end
end


if ARGV.contain?('-h') || ARGV.contain?('--help')
  puts "[--options | -o] display options window to start"
  exit 1
end
bootstrap_devices
setup_ui # init the disabled status of the buttons :)
elements[:preferences].click! if ARGV.contain?('--options') || ARGV.contain?('-o')