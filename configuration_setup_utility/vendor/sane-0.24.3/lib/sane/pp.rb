# stolen from pp.rb
# load 'pp' library on demand
module Kernel
  private
  def pp(*objs)
     require('pp')
     pp(*objs) # use the new method
  end unless respond_to? :pp 

end
