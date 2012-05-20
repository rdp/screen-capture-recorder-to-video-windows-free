require 'rubygems'
require 'sane'
require 'webrick'
p 'current:', Socket.get_local_ips
s = WEBrick::HTTPServer.new(:Port => 9090, :DocumentRoot => Dir.pwd); trap('INT') { s.shutdown }; s.start

