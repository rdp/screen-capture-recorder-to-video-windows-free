
# more helpers
class Object

  # ex: assert(some statement)
  # or
  # assert(some statement, "some helper string")
  def assert(should_be_true, string = nil)
    if(!should_be_true)
      raise "assertion failed #{string}"
    end
  end unless respond_to? :assert
  
  def refute(this_boolean, string = nil)
    assert(!this_boolean, string)
  end
  alias :y! :assert
  alias :yes! :assert
  
end
