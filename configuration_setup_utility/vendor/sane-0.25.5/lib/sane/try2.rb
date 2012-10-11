 class Object
    def try2
      self
    end
  end

  if RUBY_VERSION < '1.9.0'
  class NilClass
    def try2
      obj = Object.new # basic object?
      def obj.method_missing(*)
        nil
      end
      obj
    end
  end
  else
    class NilClass
    def try2
      obj = BasicObject.new
      def obj.method_missing(*)
        nil
      end
      obj
    end
  end
  end