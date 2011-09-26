class A
  def go
    undef :go
    def go
      3
    end
  end
end
A.new.go
