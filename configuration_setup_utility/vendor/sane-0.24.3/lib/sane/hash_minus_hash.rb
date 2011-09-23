class Hash
  # - should not affect the original
  # nor delete if the key + value pair aren't equal
  def -(hash)
    out = self.dup
    hash.keys.each do |key|
      out.delete key if self[key] == hash[key]
    end
    out
  end
end