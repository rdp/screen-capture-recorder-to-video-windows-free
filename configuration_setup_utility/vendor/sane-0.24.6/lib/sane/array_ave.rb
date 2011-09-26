module Enumerable
 def ave
   sum = 0
   self.each{|value| sum += value}
   return sum.to_f/self.length
 end
end