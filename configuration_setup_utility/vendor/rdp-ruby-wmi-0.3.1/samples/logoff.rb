require 'ruby-wmi'

LOGOFF = 0

host = 'computername'
host = WMI::Win32_OperatingSystem.find(:first, :conditions => {:primary => true}, :host => host)
host.win32shutdown(LOGOFF)

