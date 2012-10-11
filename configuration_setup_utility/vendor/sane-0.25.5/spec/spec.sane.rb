raise 'must be in spec dir' unless File.basename(Dir.pwd) == 'spec'
$:.unshift File.expand_path('../lib')
require 'rubygems'
require File.dirname(__FILE__) + '/../lib/sane.rb'
begin
  require 'spec/autorun' 
rescue LoadError
  require 'rspec'
  require 'rspec/autorun'
end
require 'fileutils'


class Object
  alias :yes :should   # a.yes == [3]
  def yes!
    self
  end
end

class FalseClass
  def yes!
    raise 'failed' # a.true!
  end
end

describe Sane do

  before do
    #Object.send(:remove_const, 'Klass') rescue nil
  end

  it "should have working __DIR__" do
    __DIR__.should_not == nil
    __DIR__[-1].should_not == '/' # not ending slash, that feels weird in real use.
  end

  it "should write to files" do
   filename = __DIR__ + '/test'
   File.write(filename, "abc\n")
   assert(File.exist?(filename))
   if RUBY_PLATFORM =~ /mswin|mingw/
     assert(File.binread(filename) == "abc\r\n") # it should have written it out *not* in binary mode
   end
   File.delete filename
  end

  class A
    def go; 3; end
  end

  it "should have a singleton_class method" do
    class A; end
    A.singleton_class.module_eval { def go; end }
    A.go
  end

  it "should have a binread method" do
    File.open("bin_test", "wb") do |f|; f.write "a\r\n"; end
    assert File.binread("bin_test") == "a\r\n"
  end  

  it "should have a binwrite method" do
   File.binwrite 'bin_test', "a\r\n"
   assert File.binread("bin_test") == "a\r\n"
  end

  it "should hash hashes right" do
    a = {} # this fails in 1.8.6, or works, rather

    a[{:a => 3, :b => 4}] = 3
    assert a[{:b => 4, :a => 3}] == 3
    assert a[{:b => 3, :a => 4}] == nil
    a = {:a => 3}
    a - {:a => 4}
    assert a.length == 1
 
  end

# my first implementation of this was *awful* LOL
# leave out for now
# just use enumerator.to_a
#  it "should allow for brackets on enumerators" do
#    require 'backports' # ugh
#    assert "ab\r\nc".lines[0] == "ab\r\n"
#  end

  it "should have verbose looking float#inspect" do
    (1.1 - 0.9).inspect.should include('0.2000000') # 0.20000000000000006661 or something close to it
    1.1.inspect.should == "1.1"
    1.0.inspect.should == "1.0"
    
  end

  it "should return false if you call File.executable? non_existent_file" do
    assert !File.executable?('nonexistent')
  end

  it "should have ave method" do
    [1,2,3].ave.should == 2
  end

  it "should have a pps method " do
    pps 1,2,3
  end

  it "should allow for map_by" do
   ["1"].map_by(:to_i).should == [1]
   ["1"].collect_by(:to_i).should == [1]
   a = ["1"]
   a.map_by!(:to_i)
   a.yes == [1]
   a == [1]
  end
  
  it "should allow for contain? and include?" do
    assert "a".include?( "a")
    assert "a".contain?( "a")
    assert !("a".contain? "b")
  end

  it "should have include and contain for arrays" do
    assert ['a'].include?( "a")
    assert ['a'].contain?( "a")
  end

  it "should have blank? and empty? for arrays and strings" do
    assert ''.blank?
    assert ''.empty?
    assert [].blank?
    assert [].empty?
  end

  it "should have a select!" do
   a = [1,2,3].select!{|n| n < 3}
   a.length.should == 2
   a = [1,2,3]
   a.select!{|n| n < 3}
   a.length.should == 2
  end

  it "should have File.filename" do
    File.filename("abc/ext.xt").should == "ext.xt"
  end
  
  it "should have an auto-loading pp method" do
    pp 1,2,3
    pp 1,2,3
  end
  
  it "should work if you require 'pp' before hand" do
    system("ruby -v -I../lib files/pp_before_hand.rb").should be_true    
  end
  
  it "should have require_relative" do
    FileUtils.touch 'go.rb'    
    require_relative 'go.rb'
    FileUtils.rm 'go.rb'
  end
  
  it "should be able to require from a subdir" do
   require_relative 'subdir/go.rb'
  end
  
  it "should require_relative after changing dirs" do
    assert system(OS.ruby_bin + " req_rel_bug/recreate.rb")
  end
  
  it "should have a File.home method" do
    assert File.home == File.expand_path('~')
  end
  
  require 'benchmark'
  
  it "should Thread.join_all_others" do
    a = Thread.new { sleep 1 }
    assert Benchmark.realtime{Thread.join_all_others} > 0.5
  end

  it "should have an insert commas operator on numbers" do
    1_000_000.group_digits.should == '1,000,000'
    1_000_000.0.group_digits.should == '1,000,000.0'
    1_000_000.03555.group_digits.should == '1,000,000.03555'
  end
  
  it "should have a string replace method" do
    "abc".replace_all!("def").should == "def"
  end
  
  it 'should have socket get_host_ips' do
    Socket.get_local_ips[0].should_not be nil
  end
  
  it 'should have present? method' do
    assert !(''.present?)
    assert 'a'.present?
    assert !([].present?)
    assert ['a'].present?
    assert !(nil.present?)
  end
  
  it 'should have Array#sample' do
    [2].sample.should == 2  
  end
  
  it 'should have String#last' do
    'abc'.last(2).should == 'bc'
	  'abc'.last(3).should == 'abc'
	  'abc'.last(1).should == 'c'
	  'abc'.last(0).should == '' # this is feeling weird now...
	  'abc'.last(44).should == 'abc'
  end
  
  it "should have a try2 method" do
    'a'.try2.gsub(/a/, 'b').should == 'b'
    nil.try2.gsub(/a/, 'b').should == nil
    proc { false.try2.gsub(/a/, 'b')}.should raise_exception(NoMethodError) # why would you want try on false? huh?
  end
end
