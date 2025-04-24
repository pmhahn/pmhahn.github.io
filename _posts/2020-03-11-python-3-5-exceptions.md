---
title: 'Python 3.5: exceptions'
date: '2020-03-11T10:19:27+01:00'
layout: post
categories: python
---

Folgendes ist mit Python 3 syntaktisch nicht mehr erlaubt:
```python
try: pass
except:
 pass

try: pass
except ExceptionType, var:
 pass

try: pass
except ExceptionType as (a, b, c):
 pass
```

Statt dessen sollte man die neue Syntax verwenden:
```python
try: pass
except Exception:
 pass

try: pass
except ExceptionType1 as var:
 pass

try: pass
except (ExceptionType1, ExceptionType2) as var:
 pass
```

Eine wichtige Änderung in Python 3 ist auch, das `var` nach der Abarbeitung des Exception-Blocks **gelöscht** wird!
Das entspricht in Python 2 also in etwa folgendem Block:
```python
try: pass
except ExceptionType1 as var:
 try:
   pass
 finally:
   del var
```

Hintergrund für die Umstellung war u.a. der, dass Exceptions eine Referenz auf den Stacktrace enthalten.
Damit ist nicht die textuelle Ansicht gemeint, sondern Referenzen auf alle Python-internen Datenstrukturen für Stack-Frames und ähnliches.
Diese enthalten wiederum Referenzen auf alle lokale Variablen der aufgerufenen Funktionen, was u.a. dazu führt, dass diese noch nicht durch den Garbage-Collector freigegeben werden können.
Dieses Verhalten auch auch schon in UMC zu einem [Memory-Leak](https://forge.univention.org/bugzilla/show_bug.cgi?id=47114) geführt.
Möchte man also nach der Behandlung einer Ausnahme noch auf diese zugreifen, muss man eine Referenz nun **explizit** darauf anlegen.

## args

In Python 2 konnte man man auch direkt über Exception iterieren:
```python
try: pass
except ExceptionType as (a, b, c):
 pass
```
Das funktioniert mit Python 3 nicht mehr und mann muss explizit auf `args` zugreifen:
```python
try: pass
except ExceptionType as ex:
 (a, b, c) = ex.args
 pass
```

## re-raise

`raise "String"` ist schon lange nicht mehr erlaubt.
Mit Python 3 ist aber auch ein `raise Exception, Exception(), Traceback()` nicht mehr erlaubt.
Ersatz bietet [six.reraise](https://six.readthedocs.io/#six.reraise).
Ein einfaches `raise` zu weiterreichen der letzten Ausnahme funktioniert aber weiterhin.

Alternativ kann man eine neue Exception werfen und die alte als Ursache anhängen: `raise NewException() from ex`.
