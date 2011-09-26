# require some "necessary" other gems

# shouldn't need this next line
# require 'rubygems' if RUBY_VERSION < '1.9' # for the other requires

require 'os'
#require 'andand' # hmm...
for file in Dir[File.dirname(__FILE__) + '/sane/*.rb'].sort do
   require file
end

class Sane
 # helper for installing it locally on 1.8
 # if you want to be able to use it without rubygems
 # except, then you'll still need rubygems anyway...
 # I hate 1.8
 def self.install_local!
   if RUBY_VERSION >= '1.9'
     raise 'you dont need to install local for 1.9!'
   end
   require 'fileutils'
   require 'rbconfig'

   Dir.chdir File.dirname(__FILE__) do
     for file in Dir['*'] do
       FileUtils.cp_r file, Config::CONFIG['sitelibdir'] # copy subdirs, too
     end
     installed_sane =  Config::CONFIG['sitelibdir'] + '/' + 'sane.rb'
     old_contents = File.read installed_sane
     File.write installed_sane, "require 'faster_rubygems'\n" + old_contents
     puts 'hardened sane with this gem version (and faster rubygems) to ' + installed_sane

   end

 end

end


