# use vendored ffmpeg
require 'sane'
ENV['PATH'] = File.dirname(__FILE__) + '\vendor\ffmpeg\bin;' + ENV['PATH']

module FfmpegHelpers
  def self.enumerate_directshow_devices
    ffmpeg_list_command = "ffmpeg.exe -list_devices true -f dshow -i dummy 2>&1"
    enum = `#{ffmpeg_list_command}`
    unless enum.present?
      p 'failed', enum
	  enum = `#{ffmpeg_list_command}`
      out = '2nd try resulted in :' + enum
      p out
      #raise out # jruby and MRI both get here???? LODO...
    end

    audio = enum.split('DirectShow')[2]
    raise enum.inspect unless audio
    video = enum.split('DirectShow')[1]
    audio_names = audio.scan(/"([^"]+)"/).map{|matches| matches[0]}
    video_names = video.scan(/"([^"]+)"/).map{|matches| matches[0]}
    {:audio => audio_names, :video => video_names}
  end
  
  def self.warmup_ffmpeg_so_itll_be_disk_cached 
    ffmpeg_list_command = "ffmpeg.exe -list_devices true -f dshow -i dummy 2>&1"
    enum = `#{ffmpeg_list_command}`
  end
end
