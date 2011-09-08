require 'win32ole'

class WIN32OLE
  include Enumerable

  # make #methods more friendly
  alias :original_methods :methods
  def methods
    original_methods + properties_.collect{|p| p.name}
  end
end
