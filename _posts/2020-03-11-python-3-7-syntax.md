---
layout: post
title: 'Python 3.7: Syntax'
date: '2020-03-11T11:07:11+01:00'
categories: Python
excerpt: "Mit Python 3 ist `print()` eine Funktion und andere Funktionen sind verschoben oder haben eine semantische Änderung erfahren."
---

## print

`print` ist ab Python 3 eine Funktion:
```python
# Python 2:
print "text"
print "text",
print "text1", "text2"
print >> sys.sdterr, "text"
# Python 3
print("text")
print("text", end=")
print("text1", "text2")
print("text", file=sys.stderr)
```

Mit einem `from __future__ import print_function` kann man die Umstellung auch schon zu Python 2-Zeiten machen.

## [Moved](https://portingguide.readthedocs.io/en/latest/builtins.html)

- `raw_input()` → `inpupt()` – das alte `input()` aus Python 2 gibt es nicht mehr.
- `file()` → `open()`
- `apply()` → `[func(_) for _ in iterable]` oder ähnliches.
- `reduce()` → `functools.reduce()`
- `exec` ist nun auch eine Funktion `exec(code, globals, locals)`
- `execfile()` …
- `reload()` → `importlib.reload()`
- `xrange()` → `range()`

Auch wurden viele Module aus der [Standardbibliothek](https://docs.python.org/3/library/index.html) aufgeräumt und [Verschoben](https://portingguide.readthedocs.io/en/latest/stdlib-reorg.html).
Entweder man verwendet [six.moves](https://six.readthedocs.io/#module-six.moves) oder so etwas wie
```python
try:
 # Python 3
 from urllib.parse import urlparse, urlencode
 from urllib.request import urlopen, Request
 from urllib.error import HTTPError
except ImportError:
 # Python 2
 from urlparse import urlparse
 from urllib import urlencode
 from urllib2 import urlopen, Request, HTTPError
```

{% include abbreviations.md %}
