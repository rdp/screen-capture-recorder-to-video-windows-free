# load bundled gems
for dir in Dir[File.dirname(__FILE__) + "/vendor/*/lib"]
  $: << dir
end

# this one isn't vendored right yet ... :P
$: << 'jruby-swing-helpers/lib'
$: << 'jruby-swing-helpers/lib/simple_gui_creator' # for ffmpeg_helpers, which we don't auto-add

# use vendored ffmpeg
ENV['PATH'] = File.dirname(__FILE__) + '\vendor\ffmpeg\bin;' + ENV['PATH']

