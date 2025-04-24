---
layout: post
title: "Python rich comparisons revisited"
date: 2023-12-18 11:46:00  +0100
categories: python
excerpt_separator: <!--more-->
---

In my previous blog post [Python rich comparison]({% post_url 2020-09-01-python-ordering %}) I looked at simplifying the comparison of objects.

Using [NotImplemented](https://docs.python.org/3/whatsnew/3.9.html#deprecated) in boolean context has been deprecated since Python 3.9:
As `bool(NotImplemented) is True` this resulted in many wrong implementations, including mine.

So how do you correctly implement rich comparison?

<!--more-->

While listening to the episode [Talk Python 441: Python = Syntactic Sugar?](https://talkpython.fm/episodes/show/441/python-syntactic-sugar) I learned something new:
CPython already does some internal optimizations which saved you from re-implementing this again in Python yourself.
Brett Cannon wrote several blog post about [Python's syntactic sugar](https://snarky.ca/tag/syntactic-sugar/).
There are two very important posts about this:
- [Unravelling binary arithmetic operations in Python](https://snarky.ca/unravelling-binary-arithmetic-operations-in-python/)
- [Unravelling rich comparison operators](https://snarky.ca/unravelling-rich-comparison-operators/)

My most important learnings from them are:

- if *left hand side* (LHS) and *right hand side* (RHS) are of the same type, then CPython will only ever call the comparison method if the LHS, e.g. `1 < 2` will result in only `int.__lt__(1,2)` being called, not also `int.__ge__(2, 1)`.

- if one argument is a true sub-type of the other, CPython will ask the sub-type first to compare itself to the super-type, e.g. `1 < my_int(2)` is automatically translated to `my_int.__ge__(2, 1)`.
  So if you derive your class `my_subclass` from the super-class `super_class`, make sure your code is able to compare to `super_class`.

Depending on what you compare, the function should `raise TypeError` when incompatible types are compared, but `return NotImplemented` if the types are compatible.

For example comparing *apples* with *oranges* should return a `TypeError`:

```python
>>> 1 == None
False
>>> 1 < None
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
TypeError: '<' not supported between instances of 'int' and 'NoneType'

>>> "" == 0
False
>>> "" < 0
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
TypeError: '<' not supported between instances of 'str' and 'int'

>>> object() == True
False
>>> object() < True
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
TypeError: '<' not supported between instances of 'object' and 'bool'
```

On the other hand some floating point numbers are integers, but in Python neither is a sub-type of the other:

```python
>>> int(5) == float(5.0)
True
>>> int.__eq__(5, 5.0)
NotImplemented
>>> float.__eq__(5.0, 5)
True
```

Please note that [namedtuple](https://docs.python.org/3/library/collections.html#collections.namedtuple) are a sub-class of `tuple`, so `namedtuple().__eq__` is used when comparing an instance to a `tuple`:

```python
>>> from collections import namedtuple
>>> Named = namedtuple("Named", "x y")
>>> Named.__bases__
(<class 'tuple'>,)
>>> plain = (1, 2)
>>> named = Named(*plain)
>>> Named.__eq__(named, plain)
True
```

{% include abbreviations.md %}
