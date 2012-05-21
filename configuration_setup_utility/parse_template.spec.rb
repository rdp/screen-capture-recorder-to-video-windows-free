require 'rubygems'
require 'rspec/autorun'

require 'parse_template.rb'
describe ParseTemplate do

  it "should parse titles" do
    frame = ParseTemplate.parse_string " ------------A Title-------------------------------------------"
	frame.get_title.should == "A Title"
  end

end