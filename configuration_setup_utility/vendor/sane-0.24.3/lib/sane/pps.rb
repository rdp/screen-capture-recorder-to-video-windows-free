class Object
  # a method that outputs several items on one line
  # similar to using pp, this space separates the items passed in and writes them to one line
  # sputs 1,2,3
  # => 1 2 3
  def pps *args
    for arg in args
      out = arg.to_s
      print out
      print " " if out[-1..-1] != " "
    end
    puts
  end
end