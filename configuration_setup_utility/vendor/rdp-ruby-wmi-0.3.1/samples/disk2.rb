# example from http://msdn.microsoft.com/en-us/library/ms974615.aspx
require 'ruby-wmi'

c_drive = WMI::Win32_PerfRawData_PerfDisk_LogicalDisk.find :first, :conditions => {'Name' => 'C:'}
puts 'C percent free:', c_drive.PercentFreeSpace.to_f*100/c_drive.PercentFreeSpace_Base
