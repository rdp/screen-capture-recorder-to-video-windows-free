require 'win32/registry'

#
# be careful using this with threads...
# if you have ever used win32ole on any previous thread, then you can use class within *that* same thread. 
# Weird, I know, but that's how win32ole works apparently.
# 
class SetupScreenTrackerParams
  Settings = ['capture_height', 'capture_width', 'start_x', 'start_y', 'default_max_fps', 
    'stretch_to_width', 'stretch_to_height',  'stretch_mode_high_quality_if_1',
    'hwnd_to_track', 'disable_aero_for_vista_plus_if_1', 
	'track_new_x_y_coords_each_frame_if_1',
    'capture_mouse_default_1',
    'capture_foreground_window_if_1',	
	'dedup_if_1', 'millis_to_sleep_between_poll_for_dedupe_changes',
	'capture_transparent_windows_including_mouse_in_non_aero_if_1_causes_annoying_mouse_flicker',
    "hwnd_to_track_with_window_decoration",
	"capture_particular_display_number_starting_at_zero"
  ]
 
   def delete_single_setting name
    with_reg do
      name = name.to_s # allow for name to be a symbol
      assert_valid_name(name)
      begin
	    @screen_reg.delete(name)
	  rescue Win32::Registry::Error => e
	    raise unless e.to_s =~ /The system cannot find the file specified./
	  end
    end
    raise if read_single_setting name
  end
  
  # pass in 0 for the value, to reset
  def set_single_setting name, value
    with_reg do
      name = name.to_s # allow for name to be a symbol
      assert_valid_name(name)
      raise unless value.is_a? Fixnum
    	p 'warning, got a negative, hope you really meant that...' if value < 0 # negatives work, but may be unexpected...
      @screen_reg.write(name, Win32::Registry::REG_DWORD, value)
    end
    set_value = read_single_setting name
    raise 'unable to set?' + name + ' remained set as ' + set_value.to_s unless set_value == value
  end
  
  def assert_valid_name(name)
    raise "unknown name #{name}" unless Settings.include?(name)
  end
  
  def convert_from_dword_to_int out
	if out > (1<<31) # appears to underflow? like 4294967288 for -1, or maybe i should just disallow negatives?
  	  p 'warning, got a negative, may not be expected...'
      # convert to a ruby negative
  	  out -= (1<<32)
	end
	out
  end
  
  def convert_from_int_to_dword out
    if out < 0
	  out += (1<<32)
	end
	out
  end
  
  # will return nil if not set in the registry...or if set to "default" value in the registry, which is 0
  def read_single_setting name
    with_reg do
      name = name.to_s # allow for name to be a symbol
      assert_valid_name(name)
      out = @screen_reg[name] rescue nil # could be a string or dword
  	  if out 
	    if out.is_a?(Fixnum)
     	  out = convert_from_dword_to_int out
  	    end
      end
	  out
    end
  end
  
  private
  
  def with_reg
    raise 'double screen reg?' if @screen_reg # unexpected
    @screen_reg = Win32::Registry::HKEY_CURRENT_USER.create "Software\\screen-capture-recorder" # LODO report: .keys method fails?
    begin
      yield
    ensure
      @screen_reg.close # surely GC will do this for me right, if I don't call teardown? Hope so...
      @screen_reg = nil
    end
  end
  
end

if $0 == __FILE__
  p 'normally you dont want to run this file!'
  incoming = Integer(ARGV[0])
  p SetupScreenTrackerParams.new.convert_from_dword_to_int incoming
  p SetupScreenTrackerParams.new.convert_from_int_to_dword incoming
end