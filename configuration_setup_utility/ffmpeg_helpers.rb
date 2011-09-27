# use vendored ffmpeg
require 'sane'
ENV['PATH'] = File.dirname(__FILE__) + '\vendor\ffmpeg\bin;' + ENV['PATH']

module FfmpegHelpers
  def self.enumerate_directshow_devices
    ffmpeg_list_command = "ffmpeg.exe -list_devices true -f dshow -i dummy 2>&1"
    enum = `#{ffmpeg_list_command}`
    unless enum.present?
      p 'failed', enum
      raise `2nd try: #{ffmpeg_list_command}`
    end

    audio = enum.split('DirectShow')[2]
    raise enum.inspect unless audio
    video = enum.split('DirectShow')[1]
    audio_names = audio.scan(/"([^"]+)"/).map{|matches| matches[0]}
    video_names = video.scan(/"([^"]+)"/).map{|matches| matches[0]}
    {:audio => audio_names, :video => video_names}
  end
end
