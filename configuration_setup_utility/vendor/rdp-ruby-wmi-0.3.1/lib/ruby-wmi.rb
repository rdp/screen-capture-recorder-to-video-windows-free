$:.unshift(File.dirname(__FILE__)) unless
  $:.include?(File.dirname(__FILE__)) || $:.include?(File.expand_path(File.dirname(__FILE__)))

require 'ruby-wmi/core_ext'
require 'ruby-wmi/constants'
require 'ruby-wmi/base'
