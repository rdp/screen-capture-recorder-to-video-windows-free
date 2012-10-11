require 'rubygems'
require '../lib/sane'
Dir.chdir File.dirname(__FILE__)
require_relative '../subdir2/require_relative2'
raise unless $was_in_rr_1 == 1
