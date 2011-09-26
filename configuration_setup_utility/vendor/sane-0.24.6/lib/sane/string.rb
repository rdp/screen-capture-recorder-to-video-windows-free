class String
 alias :blank? :empty?

 def replace_all! this_string
   gsub!(/^.*$/, this_string)  
 end

end

