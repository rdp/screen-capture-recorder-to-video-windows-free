class File
  def self.home
    File.expand_path('~') # TODO use more ENV variables [?]
  end
end