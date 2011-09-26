# stolen originally from enumerable-extra
module Enumerable  
   # Returns a new array containing the results of running +method+ once for
   # every element in the enumerable object. If both arguments and a block
   # are provided the arguments are processed first, then passed to
   # the block.
   #
   # If no method is provided, then it behaves as the standard MRI method.
   #
   # Examples:
   #
   #  array = ['foo', 'bar']
   #
   #  # No arguments
   #  array.map_by(:capitalize) => ['Foo', 'Bar']
   #
   def map_by(method)
         array  = []
         method = method.to_sym unless method.is_a?(Symbol)

         each{ |obj|
            temp = obj.send(method)
            array << temp
         }
         array      
   end

   # Reset the aliases
   alias collect_by map_by
end

class Array
   # Returns a new array containing the results of running +block+ once for
   # every element in the +array+.
   #
   # Examples:
   #
   #  array = ['foo', 'bar']
   #
   #  # No arguments
   #  array.map_by(:capitalize) => ['Foo', 'Bar']
   #
   #  # With arguments and a block
   #  array.map_by(:capitalize)
   #--
   # The Array class actually has its own implementation of the +map+ method,
   # hence the duplication.
   # 
   def map_by(method)
         array  = []
         method = method.to_sym unless method.is_a?(Symbol)
         each{ |obj|
            temp = obj.send(method)
            array << temp
         }
         array
   end

   # Same as Array#map, but modifies the receiver in place. Also note that
   # a block is _not_ required. If no block is given, an array of values
   # is returned instead
   #
   def map_by!(method)
      method = method.to_sym unless method.is_a?(Symbol)
      i = 0
      length = self.size
      while(i < length)
        self[i] = self[i].send(method)
        i+=1
      end
      self
   end

   # Set the aliases
   alias collect_by map_by
   alias collect_by! map_by!   
end
