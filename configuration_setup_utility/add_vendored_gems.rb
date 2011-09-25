# load bundled gems
for dir in Dir[File.dirname(__FILE__) + "/vendor/*/lib"]
  $: << dir
end
