if defined? IRB

  begin
    require 'irb/completion' # this one can fail if readline isn't installed
  rescue Exception
  end
  begin
    IRB.conf[:AUTO_INDENT] = true
    IRB.conf[:PROMPT_MODE] = :SIMPLE 
    #require 'irb/ext/save-history' # this one might mess with peoples' local settings of it tho...
  rescue Exception
    # guess this was some *other* IRB module? defined elsewhere?
  end

end
