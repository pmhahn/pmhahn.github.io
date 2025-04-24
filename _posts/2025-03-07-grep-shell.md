---
title: Find shell scripts
date: '2025-03-07T12:55:00+01:00'
layout: post
categories: linux git
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

Then `#!/bin/sh` plus any other shell like _Almquist shell_ `ash`, _Debian Almquist Shell_ `dash` and _GNU Bourne-Again SHell_ `bash`.
You might want to also match _C-Shell_ `csh`, _TENEX C-Shell_ `tcsh` and _Z-Shell_ `zsh`.

There may be blanks between `#!` and `/bin/sh`.

Instead of `git grep` you also can use `grep -r` itself, but as I have many git repositories, `git grep` is more efficient as it can use the index.

# Issues

This will not find _shell libraries_, e.g. _shell scripts_ not starting with a hash-bang line.

{% include abbreviations.md %}
