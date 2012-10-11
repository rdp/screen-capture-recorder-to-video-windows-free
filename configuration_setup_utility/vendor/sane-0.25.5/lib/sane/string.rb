class String
 alias :blank? :empty?

 def replace_all! this_string
   gsub!(/^.*$/, this_string)  
 end
 
 def last n
   if n > 0
     out = self[-n..-1]
   else
     out = '' # case 0 or less
   end
   out ||= self.dup # I guess .dup is good...
	 
 end

end

