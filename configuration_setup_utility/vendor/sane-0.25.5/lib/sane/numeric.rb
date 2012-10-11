
 class Numeric
   def group_digits(sep=",")
     self.to_s.reverse.scan(/(?:\d*\.)?\d{1,3}-?/).join(sep).reverse
   end
 end