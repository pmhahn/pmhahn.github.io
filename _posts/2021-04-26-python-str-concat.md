---
layout: post
title: "Pythong string concatenation"
date: 2021-04-26 12:49:00  +0200
categories: python
excerpt_separator: <!--more-->
---

How fast is string concatenation in Python?

```python
a + b + c
"".join((a, b, c))
"%s%s%s" % (a, b, c)
"{}{}{}".format(a, b, c)
f"{a}{b}{c}"
c="";c+=x;c+=y;c+=z;c
```

<!--more-->

Test
====

Let's test this with a small program:

```python
from timeit import timeit

V = (
    'x + y + z',
    '"".join((x, y, z))',
    '"%s%s%s" % (x, y, z)',
    '"{}{}{}".format(x, y, z)',
    'f"{x}{y}{z}"',
    # 'c="";for s in (x,y,z):c+=s',
    'c="";c+=x;c+=y;c+=z;c'
)

for bit in (0, 4, 8, 10, 12, 16):
    size = 1 << bit
    x, y, z = (c * size for c in "xyz")
    var = dict(V=V, x=x, y=y, z=z)
    print(f"size={size}")
    times = sorted((timeit(v, number=1000000, globals=var), v) for v in V)
    for (d, v) in times:
        print(f"\t{d:.3f}\t+{100*(d/times[0][0]-1):3.0f}%\t{v:s}")
```

Results
=======

This is with *Python 3.7* on *Intel® Core™ i5-6200U CPU @ 2.30GHz*:

| Variant / Length           |     1 |    16 |   256 |    1k |    4k |   64k |
|----------------------------|-------|-------|-------|-------|-------|-------|
| `f"{x}{y}{z}"`             | 0.102 | 0.096 | 0.118 | 0.166 | 0.356 | 7.599 |
| `x + y + z`                | 0.120 | 0.135 | 0.170 | 0.192 | 0.325 | 7.680 |
| `"".join((x, y, z))`       | 0.136 | 0.135 | 0.153 | 0.203 | 0.416 | 7.703 |
| `c="";c+=x;c+=y;c+=z;c`    | 0.141 | 0.151 | 0.202 | 0.218 | 0.389 | 7.819 |
| `"%s%s%s" % (x, y, z)`     | 0.184 | 0.181 | 0.281 | 0.313 | 0.515 | 7.733 |
| `"{}{}{}".format(x, y, z)` | 0.251 | 0.252 | 0.357 | 0.384 | 0.641 | 7.882 |

Conclusion
==========

**Currently** f-strings seems to be the fastest.
The historical recommendation to not use `+` seems to be out-dated, at least for the case of a limited set of strings to concatenate.

Links
=====

* [An Optimization Anecdote](https://www.python.org/doc/essays/list2str/)
* [Stackoverflow: What is the most efficient string concatenation method in Python](https://stackoverflow.com/questions/1316887/what-is-the-most-efficient-string-concatenation-method-in-python)
