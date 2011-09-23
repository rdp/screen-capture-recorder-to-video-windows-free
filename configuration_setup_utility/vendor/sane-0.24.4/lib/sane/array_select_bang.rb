class Array
 def select!
  where_at = 0
  total = length
  each{|member|
    if yield(member)
      self[where_at] = member
      where_at += 1
    end
  }
  while(where_at < total)
    self[where_at] = nil
    where_at += 1
  end
  compact! # there may be a better way...
  self
 end
end