require 'benchmark'

out = []
for threads in [1,2]
  for type in ['copy', 'png', 'qtrle', 'ffv1', 'huffyuv', 'zlib'] # ljpeg, mjpeg, x264 lossless
    elapsed = Benchmark.realtime{
	  c  = "ffmpeg -threads #{threads} -y -i a10.avi -acodec libmp3lame -vcodec #{type} affv1.mov"
	  puts
	  puts c
      system(c)
    }
	puts
    nexty = "#{type} time: #{elapsed} size: #{File.size 'affv1.mov'} threads #{threads}\n"
	print nexty
	out << [type,elapsed, File.size('affv1.mov'), threads]
  end
end
puts
puts
require 'pp'
pp out
puts out
puts out.sort_by{|n, t, s, th| t}.map{|name, time, size, threads| [name, ((time*10).to_i/10.0).to_s + 's', "#{size/1000000}M", threads]}