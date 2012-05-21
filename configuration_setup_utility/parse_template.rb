require 'java'

class ParseTemplate

  def self.parse_filename filename
    parse_string File.read(filename)
  end
  
  # returns a jframe that "matches" whatever you put  
  def self.parse_string string
    got = string
 #>>"[ a ] [ b ]".split(/(\[.*?\])/)
 # => ["", "[ a ]", " ", "[ b ]"]	
    frame = java.awt.Frame.new
    got.each_line{|l|
	  if got =~ /\s*[-]+([\w ]+)[-]+\s*$/ # ----(a Title)---
	    frame.set_title $1
	  end
	
	  button_line_regex = /\[(.*?)\]/
	  # >> "|  [Setup Preferences:preferences] [Start:start] [Stop:stop] |" .scan  /\[(.*?)\]/
	  # => [["Setup Preferences:preferences"], ["Start:start"], ["Stop:stop"]]
      if got =~ button_line_regex # like  
        got.scan(button_line_regex).each{|name|
		  if name.include? ':' # like "Start:start_button"
		    text = name.split[0]
			name = name.split[1]
		  else
		    text = name
		  end
		  
		}
	    
	  end
	}
	frame
  end
  
end