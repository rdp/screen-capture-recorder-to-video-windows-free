# to run this use c:\>ruby setup_params.rb ... (doesn't require jruby)
# or jruby setup_params.rb to have it prompt you with GUI for values and set them.

require 'win32/registry'

class SetupScreenTrackerParams
  Settings = ['height', 'width', 'start_x', 'start_y', 'default_max_fps']
 
  def initialize
    re_init
  end
  
  # some apps need to reinit if they first got the registry on a different thread...I think so anyway...
  def re_init
    @screen_reg = Win32::Registry::HKEY_CURRENT_USER.create "Software\\os_screen_capture" # LODO .keys fails?
  end
  
  def set_single_setting name, value
    name = name.to_s # allow for symbols
    raise "unknown name #{name}" unless Settings.include?(name)
    raise unless value.is_a? Fixnum # pass in 0 if you want to reset anything...
	p 'warning, got a negative' if value < 0
    @screen_reg.write(name, Win32::Registry::REG_DWORD, value)
    set_value = read_single_setting name
	set_value ||= 0 # this is a bit kludgey...
    raise 'unable to set?' + name + ' remained set as ' + set_value.to_s unless set_value == value
  end
  
  # will return nil if not set in the registry...
  def read_single_setting name
    raise "unknown name #{name}" unless Settings.include?(name)
    out = @screen_reg.read_i(name) rescue nil
	if out && out > (1<<31) # underflow? like 4294967288 for -1, or maybe i shoudl disallow negatives?
	  p 'warning, got a negative...'
	  out -=  (1<<32)
	end
    out = nil if out == 0 # handle having been set previously back to the default.
    out
  end
  
  def teardown
    @screen_reg.close # surely GC will do this for me right?
    @screen_reg = nil
  end
  
  def all_current_values
    out = {}
    for name in Settings
      out[name] = read_single_setting(name) # might be nil...
    end
    out
  end
  
end
