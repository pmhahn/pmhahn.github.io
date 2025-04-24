---
title: 'Python 3.2: __next__'
date: '2020-03-10T18:18:01+01:00'
layout: post
categories: python
---

Python 3 verwendet öfters Iteratoren als Python 2, so lieferen `dict().keys(), dict().values(), dict.items()` inzwischen Iteratoren statt Listen.
In Python 2 wurde dafür extra `dict().iterkeys(), dict().itervalues(), dict().iteritems()` hinzugefügt, die es in Python 3 nicht mehr gibt.

Das führt mit Python 3 aber dazu, das Konstrukte wie `dict().keys() + dict().keys()` nicht mehr funktioniert, weil hier jetzt zwei Iteratoren per `+` verknüpft werden soll anstatt von zwei Listen.
Man kann dafür [itertools.chain()](https://docs.python.org/3/library/itertools.html#itertools.chain) verwenden oder muss es eben explizit umschreiben in `list(dict().keys()) + list(dict().keys())`.

Auch aufpassen muss man wenn man eigene Iteratoren implementiert hat:
Früher musste man die Methode `next()` implementieren, mit Python 3 statt dessen `__next__()`.
Für für Rückwärtskompatibilität für Python 2 kann man dann einfach `next = __next__` ergänzen (und das in Zukunft dann irgendwann entfernen).

Siehe [Iterators](https://portingguide.readthedocs.io/en/latest/iterators.html#new-iteration-protocol-next) für genauere Details.
