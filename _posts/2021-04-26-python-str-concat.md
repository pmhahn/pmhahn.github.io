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
from typing import Dict, Tuple

V = (
    'x + y + z',
    '"".join((x, y, z))',
    '"%s%s%s" % (x, y, z)',
    '"{}{}{}".format(x, y, z)',
    'f"{x}{y}{z}"',
    'c="";c+=x;c+=y;c+=z;c'
)
B = (0, 4, 8, 10, 12, 16)
S = [1 << b for b in B]
T: Dict[str, Dict[int, Tuple[float, float]]] = {}

for size in S:
    x, y, z = (c * size for c in "xyz")
    var = dict(V=V, x=x, y=y, z=z)
    print(f"size={size}")
    times = sorted((timeit(v, number=1000000, globals=var), v) for v in V)
    for (d, v) in times:
        rel = 100 * (d / times[0][0] - 1)
        print(f"\t{d:.3f}\t+{rel:3.0f}%\t{v:s}")
        T.setdefault(v, {})[size] = (d, rel)

for v, s in T.items():
    row = ' | '.join(
        f"{d:.3f} +{rel:3.0f}%"
        for (d, rel) in (s[size] for size in S)
    )
    print(f"| `{v:24s}` | {row} |")
```

Results
=======

This is with *Python 3.7* on *Intel® Core™ i5-6200U CPU @ 2.30GHz*:

| Variant / Length           |           1 |          16 |         256 |          1k |          4k |         64k |
|----------------------------|-------------|-------------|-------------|-------------|-------------|-------------|
| `f"{x}{y}{z}"            ` | 0.100 +  0% | 0.098 +  0% | 0.116 +  0% | 0.173 +  0% | 0.323 +  0% | 6.964 +  0% |
| `x + y + z               ` | 0.114 + 14% | 0.127 + 30% | 0.213 + 84% | 0.192 + 11% | 0.332 +  3% | 7.023 +  1% |
| `"".join((x, y, z))      ` | 0.134 + 34% | 0.133 + 36% | 0.153 + 32% | 0.206 + 19% | 0.414 + 28% | 7.037 +  1% |
| `c="";c+=x;c+=y;c+=z;c   ` | 0.141 + 41% | 0.156 + 60% | 0.241 +108% | 0.219 + 26% | 0.396 + 23% | 6.943 +  0% |
| `"%s%s%s" % (x, y, z)    ` | 0.180 + 80% | 0.183 + 87% | 0.320 +176% | 0.309 + 78% | 0.529 + 64% | 7.097 +  2% |
| `"{}{}{}".format(x, y, z)` | 0.248 +149% | 0.282 +189% | 0.400 +245% | 0.423 +144% | 0.646 +100% | 7.305 +  5% |


Conclusion
==========

**Currently** f-strings seems to be the fastest.
The historical recommendation to not use `+` seems to be out-dated, at least for the case of a limited set of strings to concatenate.

Links
=====

* [An Optimization Anecdote](https://www.python.org/doc/essays/list2str/)
* [Stackoverflow: What is the most efficient string concatenation method in Python](https://stackoverflow.com/questions/1316887/what-is-the-most-efficient-string-concatenation-method-in-python)
