require 'java'

module ParseTemplate

  class JFramer < javax.swing.JFrame
    
    def initialize 
      super()
      @panel = javax.swing.JPanel.new
      @buttons = []
      @panel.set_layout nil
      add @panel # why can't I just slap these down? panel? huh?
	  show
	end 
	attr_accessor :buttons
	attr_accessor :panel
  end
  
  def self.parse_filename filename
    parse_string File.read(filename)
  end
  
  #       happy = Font.new("Tahoma", Font::PLAIN, 11)

  
  # returns a jframe that "matches" whatever you put  
  def self.parse_string string
    got = string
 #>>"[ a ] [ b ]".split(/(\[.*?\])/)
 # => ["", "[ a ]", " ", "[ b ]"]	
    frame = JFramer.new
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
		  button = JButton.new text
          button.tool_tip = text
          button.set_bounds(44, 33, 35, 23) # TODO test
          frame.panel.add button
          frame.buttons << button		  
		}
	    
	  end
	}
	frame
  end

end