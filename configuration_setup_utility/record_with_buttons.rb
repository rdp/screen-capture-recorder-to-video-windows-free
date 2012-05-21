require 'common_recording.rb'
require 'jruby-swing-helpers/parse_template.rb'
require 'jruby-swing-helpers/storage.rb'

frame = ParseTemplate.parse_file 'record_buttons.template'
storage = Storage.new('record_with_buttons')
require 'tmpdir'
storage['save_to_dir'] ||= Dir.tmpdir


frame.elements['record_to_dir_text'].text += " " + storage['save_to_dir']

frame.elements['reveal_save_to_dir'].on_clicked {
  SwingHelpers.show_in_explorer storage['save_to_dir']
}

frame.show