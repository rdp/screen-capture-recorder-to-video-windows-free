Thread.abort_on_exception = true # typically you *want* to know when a thread dies unexpectedly.

require 'socket'
BasicSocket.do_not_reverse_lookup = true # avoid some bugs...

# abstracted from require 'facets/file' ===>
class File

  # Writes the given data to the given path and closes the file.  This is
  # done in binary mode, complementing <tt>IO.read</tt> in standard Ruby.
  #
  # Returns the number of bytes written.
  #
  # CREDIT: facets/Gavin Sinclair

  def self.write(path, data)
    File.open(path, "w") do |file|
      return file.write(data)
    end
  end unless self.respond_to?(:write)

  def self.binwrite(path, data)
    File.open(path, "wb") do |file|
      return file.write(data)
    end
  end

end

# more helpers
class Object

  # a helper for collection.include?
  def in? collection
    collection.include?(self)
  end unless respond_to? :in

  # helper to bring up a debugger with less writing [just _dbg]
  def _dbg
    begin
      require 'ruby-debug'
      debugger
    rescue LoadError => e
      require 'rubygems'
      begin
        require 'ruby-debug'
      rescue LoadError => e
        throw "unable to load ruby-debug gem for _dbg... #{e}"
      end
    end
  end

  def singleton_class
    class << self; self; end
  end
end

# taken from http://oldrcrs.rubypal.com/rcr/show/309

module Kernel
  #  BASE_DIR = Dir.getwd
  
  def __DIR__
    dir = (/^(.+)?:\d+/ =~ caller[0]) ? File.expand_path(File.dirname($1), Dir.pwd) : nil
    dir
  end unless defined?(__DIR__)
  # below deemed too unfriendly since it would
  # seem to imply that _file__ also exists
  # alias __dir__ __DIR__ unless defined?(__dir__)
end

if RUBY_VERSION >= '1.9.2'
  $: << '.' # for some reason loading files from the cwd was taken out.  That is not sane.
end
