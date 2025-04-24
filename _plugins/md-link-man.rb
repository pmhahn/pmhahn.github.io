Jekyll::Hooks.register :documents, :pre_render do |document, payload|
  c = document.content
  c = c.gsub(/<man:([-.0-9A-Z_a-z]+)\(([1-8])\)>/, '[\1(\2)](https://man7.org/linux/man-pages/man\2/\1.\2.html)')
  c = c.gsub(/\]\(man:([-.0-9A-Z_a-z]+)\(([1-8])\)\)/, '](https://man7.org/linux/man-pages/man\2/\1.\2.html)')
  document.content = c
end
