class Thread
  def self.join_all_others
    Thread.list.each{|t| t.join unless t == Thread.current}
  end
end