Jekyll::Hooks.register :documents, :pre_render do |document, payload|
  content = document.content.gsub(/<man:([-.0-9A-Z_a-z]+)\(([1-8])\)>/, '[\1(\2)](https://man7.org/linux/man-pages/man\2/\1.\2.html)')
  document.content = content
end
