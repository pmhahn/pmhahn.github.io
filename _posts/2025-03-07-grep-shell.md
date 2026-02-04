---
title: Find shell (or Python) scripts
date: '2025-03-07T12:55:00+01:00'
layout: post
categories: shell git python
tags: find
excerpt_separator: <!--more-->
---

Q: How do I recursively find all shell scripts in my current working directory?

A: Search for the hash-bang line:

```console
$ git -c grep.fallbackToNoIndex=yes grep -lPe '\A#!\s*/bin/([bd]?a)?sh\b'
```

<!--more-->

# Explanation

I'm using _Perl Compatible Regular Expressions_ (PCRE) as this gives us `\A` to match _at the beginning of the subject_.
The circumflex (`^`) would match each line, not only the first line.

Then `#!/bin/sh` plus any other shell like _Almquist shell_ [`ash`](https://en.wikipedia.org/wiki/Almquist_shell), _Debian Almquist Shell_ [`dash`](man:dash(1)) and _GNU Bourne-Again SHell_ [`bash`](man:bash(1)).
You might want to also match _C-Shell_ [`csh`](https://en.wikipedia.org/wiki/C_shell), _TENEX C-Shell_ [`tcsh`](https://www.tcsh.org/) and _Z-Shell_ [`zsh`](https://www.zsh.org/).

There may be blanks between `#!` and `/bin/sh`.

Instead of [`git grep`](man:git-grep(1)) you also can use [`grep -r`](man:grep(1)) itself, but as I have many git repositories, `git grep` is more efficient as it can use the index.

# Issues

This will not find _shell libraries_, e.g. _shell scripts_ not starting with a hash-bang line.

For Python use this:
```console
$ git -c grep.fallbackToNoIndex=yes grep -lPe '\A#!\s*/usr/bin/(env\s+)?python'
```

{% include abbreviations.md %}
