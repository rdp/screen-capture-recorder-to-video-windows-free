class Time
  
  def to_swebmDateTime
    t = strftime("%Y%m%d%H%M%S")
    o = gmt_offset/60
    "#{t}.000000#{o}" 
  end

end