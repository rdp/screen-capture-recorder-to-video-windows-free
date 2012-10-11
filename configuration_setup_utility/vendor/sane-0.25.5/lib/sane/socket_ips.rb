class Numeric
   def ord; self; end unless RUBY_VERSION[0..2] == '1.9' # helper for 1.8/1.9
end

require 'socket' # sane already loads it for BasicSocket.reverse_lookup

class Socket
 class << self
   def get_local_ips
     get_ips(Socket.gethostname)
   end

   def get_ips hostname
     begin

       socket_info = Socket.getaddrinfo(hostname, nil,
         Socket::AF_UNSPEC, Socket::SOCK_STREAM, nil,
         Socket::AI_CANONNAME).select{|type| type[0] == 'AF_INET'}

       raise if socket_info.length == 0
       ips = socket_info.map{|info| info[3]}
       return ips
     rescue => e # getaddrinfo failed...
       begin
          ipInt = gethostbyname(hostname)[3] # do a DNS lookup
          return ["%d.%d.%d.%d" % [ipInt[0].ord, ipInt[1].ord, ipInt[2].ord, ipInt[3].ord]]
       rescue SocketError # happens in certain instances...
          return nil
       end
     end
   end

 end
end