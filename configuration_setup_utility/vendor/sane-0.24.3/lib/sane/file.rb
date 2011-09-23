class File
 def self.binread filename
   File.open(filename, 'rb')do |f| f.read; end
 end
end