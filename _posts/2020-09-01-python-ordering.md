---
layout: post
title: "Python rich comparisons"
date: 2020-09-01 12:50:00  +0200
categories: python
excerpt_separator: <!--more-->
---

Python 3 has switch to [Rich Comparisons](https://www.python.org/dev/peps/pep-0207/).
With Python 2 is was enough to implement a single `__cmp__(self, other)` method, now you have to implement
* `__lt__(self, other)`
* `__le__(self, other)`
* `__eq__(self, other)`
* `__ne__(self, other)`
* `__ge__(self, other)`
* `__gt__(self, other)`
* `__hash__(self)`

[@functools.total_ordering](https://docs.python.org/3/library/functools.html#functools.total_ordering) helps with this, but this is still painful.

<!--more-->

I often have classes, which work like [namedtuple](https://docs.python.org/3/library/collections.html#collections.namedtuple).
Previously I was done with

```python
class Base(object):
  def __init__(self, a, b):
    self.a = a
    self.b = b

  def __hash__(self):
    return hash((self.a, self.b))

class Old(Base):
  def __cmp__(self, other):
    if not isinstance(other, Base):
      return NotImplemented
    return cmp((self.a, self.b), (other.a, other.b))
```

Now I have to implement it like this:

```python
class New(Base):
  def __lt__(self, other):
    if not isinstance(other, Base):
      return NotImplemented
    return (self.a, self.b) < (other.a, other.b)

  def __le__(self, other):
    if not isinstance(other, Base):
      return NotImplemented
    return (self.a, self.b) <= (other.a, other.b)

  def __eq__(self, other) -> bool:
    return isinstance(other, Base) and (self.a, self.b) == (other.a, other.b)

  def __ne__(self, other) -> bool:
    return not isinstance(other, Base) or (self.a, self.b) != (other.a, other.b)

  def __ge__(self, other):
    if not isinstance(other, Base):
      return NotImplemented
    return (self.a, self.b) >= (other.a, other.b)

  def __gt__(self, other):
    if not isinstance(other, Base):
      return NotImplemented
    return (self.a, self.b) > (other.a, other.b)
```

This is a lot more work and also error prone due the repitition of 6 times nearly the same code with subtile differences.
I have seen wrong implementation doing it like this:

```python
class Wrong(Base):
  def __lt__(self, other):
    if self.a < other.b:
      return True
    if self.b < other.b
      return True
    return False
```

This is wrong as the test for `b` must also check for `self.a == self.b`:

```python
class StillIncomplete(Base):
  def __lt__(self, other):
    if self.a < other.b:
      return True
    elif self.a == self.b and self.b < other.b
      return True
    return False
```

This still lacks support for [NotImplemented](https://docs.python.org/3/library/constants.html?highlight=notimplemented#NotImplemented), which is required for handling the case, where a sub-class wants to implement special handling when an instance of that class is compared to a super- (or distinct) class.
(Do not confuse this with [NotImplementedError](https://docs.python.org/3/library/exceptions.html#NotImplementedError), which is an exception for **abstract** methods.)

You can write it more compact like

```python
class Ugly(Base):
  def __lt__(self, other):
    return (self.a < other.b) or ((self.a == self.b) and (self.b < other.b)) \
      if isinstance(self, Base) else NotImplemented
```

but this does not get prettier when you compare more then two values.


Re-using the idea of `total_ordering` you can write your own decorator for classes, which adds the missing functions based on some attribute:

```python
from operator import attrgetter

def ordering(getter):
  def decorator(cls):
    ops = {
      '__lt__': lambda self, other: getter(self) < getter(other) if isinstance(other, cls) else NotImplemented,
      '__le__': lambda self, other: getter(self) <= getter(other) if isinstance(other, cls) else NotImplemented,
      '__eq__': lambda self, other: isinstance(other, cls) and getter(self) == getter(other),
      '__ne__': lambda self, other: not isinstance(other, cls) or getter(self) != getter(other),
      '__ge__': lambda self, other: getter(self) >= getter(other) if isinstance(other, cls) else NotImplemented,
      '__gt__': lambda self, other: getter(self) > getter(other) if isinstance(other, cls) else NotImplemented,
      '__hash__': lambda self: hash(getter(self)),
    }
    root = set(dir(cls))
    for opname, opfunc in ops.items():
      if opname not in root or getattr(cls, opname, None) is getattr(object, opname, None):
        opfunc.__name__ = opname
        setattr(cls, opname, opfunc)

    return cls

  return decorator
```

This can be used like this:

```python
@ordering(attrgetter("val"))
class foo(object):
  def __init__(self, val):
    self.val = val

x, y = foo(1), foo(2)
assert x == x
assert not x == y
assert x != y
assert x <= x
assert x < y
assert x in {x}

class bar(foo): pass

z = bar(1)
assert x == z
assert z == x
assert z != y
assert z in {x}
```

This also work for more complex structures like *tuples*:

```python
x, y = foo((1, 1)), foo((1, 2))
z = bar((1, 1))
```

If you have a class with **multiple** components, you can implement `__iter__()` to return those parts and use them for comparison:

```python
@ordering(lambda obj: tuple(iter(obj)))
class foo(object):
  def __init__(self, a, b):
    self.a = a
    self.b = b

  def __iter__(self):
    yield self.a
    yield self.b

x, y = foo(0, "x"), foo(0, "y")
```

This works with both Python 2 and 3.
