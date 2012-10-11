# LODO move to sane :) also remove the andand dep.
class String
  def present?
    length > 0
  end
end

class NilClass
  def present?
    false
  end
end
class Object
  def present?
    true
  end
end

class Array
  def present?
    length > 0
  end
end

