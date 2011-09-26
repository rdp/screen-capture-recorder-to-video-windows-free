unless Kernel.respond_to?(:require_relative, true)
  
module Kernel
  module_function
  OriginalDir = File.expand_path(Dir.pwd) # try to accomodate for later Directory changes...
  
  def require_relative(relative_feature)
    c = caller.first
    # could be spec.sane.rb:127
    # or e:/abc.rb:127
    e = c.rindex(/:\d+/)
    file = $`
    if /\A\((.*)\)/ =~ file # eval, etc.
      raise LoadError, "require_relative is called in #{$1}"
    end
    absolute_feature = File.expand_path(File.join(File.dirname(file), relative_feature))
    begin
      require absolute_feature
    rescue LoadError => e
      # hacky kludge in case they've changed dirs...
      begin
        require File.expand_path(File.join(OriginalDir,File.dirname(file), relative_feature))
      rescue LoadError => ignore_me
        raise e # don't mask...
      end
    end
    
  end
end

end
