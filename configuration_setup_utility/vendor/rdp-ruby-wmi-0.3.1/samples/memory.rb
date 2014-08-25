require 'ruby-wmi'

properties = ['Description','MaxCapacity','MemoryDevices','MemoryErrorCorrection']


mem = WMI::Win32_PhysicalMemoryArray.find(:all, :host => 'server1')
mem.each{|i| puts properties.map{|p| "#{p}: #{i[p]}"}}

mem2 = WMI::Win32_PhysicalMemoryArray.find(:all, {:host => 'server2', :user => 'domain\\gordon', :passwd => 'password'} )
mem2.each{|i| puts properties.map{|p| "#{p}: #{i[p]}"}}

