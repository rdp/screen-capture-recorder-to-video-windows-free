require 'ruby-wmi'

disks = WMI::Win32_LogicalDisk.find(:all)

disks.each do |disk|
 disk.properties_.each do |p|
    puts "#{p.name}: #{disk[p.name]}"
  end
end