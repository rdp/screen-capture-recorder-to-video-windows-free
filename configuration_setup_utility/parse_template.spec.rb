require 'rubygems'
require 'rspec/autorun'
require 'sane' # gem

require 'parse_template.rb'
describe ParseTemplate do

  def parse_string string
    ParseTemplate.parse_string string
  end
  it "should parse titles" do
    frame = parse_string " ------------A Title-------------------------------------------"
	frame.get_title.should == "A Title"
  end
  
  it "should parse button only lines" do
   frame = parse_string "|  [Setup Preferences:preferences] [Start:start] [Stop:stop] |"
   assert frame.buttons.length == 3
   frame.buttons['preferences'].text.should == "Setup Preferences"
   assert frame.buttons['start'].text == "Start"
   frame.buttons['preferences'].text = "new text" 
   assert frame.buttons['preferences'].text == "new text"
   frame.buttons['preferences'].location.x.should_not == frame.buttons['start'].location.x
   frame.buttons['preferences'].location.y.should == frame.buttons['start'].location.y
   frame.get_size.height.should be > 0
   frame.get_size.width.should be > 0
   frame.show
  end

end
