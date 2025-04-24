---
title: 'find -printf'
date: '2017-03-01T17:30:41+01:00'
layout: post
categories: linux filesystem
---

I had to search [man 1 find](man:find(1)) too often for `printf`:

<pre style="font-family:monospace">$ find <span style="color: #00ff00">/etc</span> -name <span style="color: #ff0000">bind9</span> -printf '<span style="color: #00ff00">%H</span>/<span style="color: #00ffff">%P</span>\n<span style="color: #ff6600">%h</span>/<span style="color: #ff0000">%f</span>\n<span style="color: #0000ff">%p</span>\n'
<span style="color: #00ff00">/etc</span>/<span style="color: #00ffff">init.d/bind9</span>
<span style="color: #00ff00">└%H┘</span> <span style="color: #00ffff">└─────%P───┘</span>
<span style="color: #ff6600">/etc/init.d</span>/<span style="color: #ff0000">bind9</span>
<span style="color: #ff6600">└────%h───┘</span> <span style="color: #ff0000">└─%f┘</span>
<span style="color: #0000ff">/etc/init.d/bind9</span>
<span style="color: #0000ff">└──────%p───────┘</span>
</pre>

And `%l` for symbolic links, when `-ls` is too talkative:

```sh
find /etc/init.d -name '*bind*' -type l -printf '%p -> %l\n'
```

And the classic to find broken links:

```sh
find /etc -xtype l -printf '%p -> %l\n
```

{% include abbreviations.md %}
