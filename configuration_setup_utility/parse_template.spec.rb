require 'rubygems'
require 'rspec/autorun'

require 'parse_template.rb'
describe ParseTemplate do

  def parse_string string
    ParseTemplate.parse_string string
  end
  it "should parse titles" do
    frame = parse_string " ------------A Title-------------------------------------------"
	frame.get_title.should == "A Title"
  end
  
  it "should parse button lines" do
   frame = parse_string "|  [Setup Preferences:preferences] [Start:start] [Stop:stop] |"
   assert frame.buttons.length == 3
   assert frame.buttons[0].text == "Setup Preferences"
   assert frame.buttons[1].text == "Start"
   # introspect name?
   frame.preferences.text = "new text" 
   assert frame.buttons[0].text == "new text"
  end

end