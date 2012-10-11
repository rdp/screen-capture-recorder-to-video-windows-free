module Enumerable
 def ave
   sum = 0
   self.each{|value| sum += value}
   return sum.to_f/self.length
 end

  if RUBY_VERSION < '1.9.0'
    def sample
     me = to_a
     me[rand(me.length)]
    end
  end

end