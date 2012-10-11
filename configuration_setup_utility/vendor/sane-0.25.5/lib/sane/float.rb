if RUBY_VERSION < '1.9.2'
  class Float
    def inspect
      big = "%.20f" % self # big!
      small = "%f" % self
      
      if small.to_f == self
        small.sub!(/0+$/, '')
        small.sub(/\.$/, '.0') # reformat, see spec
      else
        big
      end
    end
  end
end