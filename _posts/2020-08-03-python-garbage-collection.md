---
title: 'Python Garbage Collection'
date: '2020-08-03T07:24:07+02:00'
layout: post
categories: python
---

Man sollte mit `__del__()`-Methoden in Python sehr aufpassen:

Der [Garbage Collector](https://docs.python.org/3/library/gc.html) (GC) von Python räumt im Gegensatz zu anderen Sprachen auch zyklische Strukturen auf, jedoch ist das deutlich komplizierter.
Normalerweise werden Objekt über die Referenzzählung freigegebenen:

> root ->; a[1] -> b[1]

Nach einem `del a` sinkt der Referenzzähler von "a" auf 0, was dazu führt, das "a" freigegeben wird. Das führt rekursiv dazu, das auch "b" freigegeben wird, weil durch das implizite `del b` auch dessen Referenzzähler auf 0 sinkt.

Bei zyklischen Strukturen ist es komplizierter:

> root -> a[2] <=> b[1]

Nach dem `del a` sinkt der Referenzzähler von "a" nur auf 1, so dass "a" und "b" sich weiterhin gegenseitig referenzieren, aber ansonsten nicht mehr erreichbar sind.
Letzteres stimmt nicht ganz, denn der Python-Interpreter hat auf **alle** derzeit allozierten Objekte eine **interne** Referenz.
Diese nutzt der "Mark-and-Sweep"-GC, um in regelmäßigen Abständen folgendes zu machen:

1. Bei allen allozierten Objekte wird das "referenced"-Flag zurückgesetzt.
2. Ausgehen von "root" wird allen Referenzen rekursiv gefolgt und bei allen so besuchten Objekten das "referenced"-Flag gesetzt.
3. Abermals ausgehend von der internen Liste aller Objekte werden nun alle Objekte freigegeben, deren "referenced"-Flag nicht gesetzt wurde.

Da alle diese Objekte Teil mindestens einer zyklischen Referenzierung sind, werden diese an einer "zufälligen" Stelle aufgebrochen.
Das geht aber nur, wenn keins der Objekte eine `__del__()`-Methode hat, denn bei diesen kann der GC nicht wissen, was diese genau tun und wie sie reagieren würden, wenn nun genau dort der Zyklus ausgebrochen wird und eine Referenz, die in `__del__()` noch benötigt würde, plötzlich `None` ist.
Das Problem ist hier, dass die `__del__()`-Methode beliebige Dinge tun kann, u.a. eben jede Menge neue Objekte anlegen kann, die dann auch mit entfernt werden müssen, oder noch viel schlimmer, den bis dahin nicht erreichbaren Teil wieder irgendwo über eine globale Variable erreichbar macht.
Zyklen mit `__del__()`-Methoden werden statt dessen in [gc.garbage](https://docs.python.org/3/library/gc.html#gc.garbage) eingehängt.
Will man diesen Speicher freigeben, muss man selber Code schreiben, der Zyklen in eigenen Datenstrukturen erkennt und diese dann _intelligent_ aufbricht.

Von daher sollte man `__del__()` sehr sparsam einsetzen und es nie dafür zu nutzen, um nur irgendwelche Referenzen auf andere Python-Objekte zu löschen:
Denn die Methode wird sowieso erst dann aufgerufen, wenn der GC sowieso schon entschieden hat, das Objekte freizugeben.
Referenzierte Objekte werden dann sowieso über den Mechanismus der Referenzzählung wie ganz zu Anfang beschrieben freigegeben.
Wenn überhaupt dient `__del__()` dazu, **externe** Ressourcen wie offene File-Deskriptoren, Speicher, temporäre Dateien, Locks, etc. freizugeben.

Da der Aufwand für den M&S-GC deutlich aufwändiger ist als Referenzzählung, wird der Algorithmus nur ab und zu ausgeführt.
Man merkt das daran, dass dann ggf. die Ausführung des eigenen Codes für eine gewisse Zeit pausiert ist.
Man kann das ganze auch explizit über [gc.collect()](https://docs.python.org/3/library/gc.html#gc.collect) triggern, falls man weiß, das der eigene Code gerade eine zyklische Struktur ausgehängt hat und diese nun aufgeräumt werden soll, bevor der eigene Code mit dem nächsten Großen Block fortfährt.
Noch effizienter ist es natürlich, zyklische Strukturen selber aufzubrechen, denn dann erfolgt die Freigabe über den Referenzzähler.

Mit [PEP 442](https://www.python.org/dev/peps/pep-0442/) gibt es seit Python 3.4 nochmals eine Änderung in der Implementierung, die auch das Aufräumen von Zyklen mit `__del__()`-Methoden ermöglicht:
Dazu wurde die Ausführung der `__del__()`-Methoden von der eigentlichen Freigabe der Objekte getrennt.

Bzgl. Referenzen auf [Exceptions](https://docs.python.org/3/reference/compound_stmts.html#the-try-statement) ist folgendes zu ergänzen:
```python
try:
 …
except Exception as ex:
 code(ex)
```

Mit Python 2 konnte man über `sys.exc_info()[2]` an den kompletten Traceback kommen, der u.a. die Stack-Frames aller verschachtelt aufgerufenen Funktionen enthält, in der der eigentliche Fehler dann tief verschachtelt auftrat.
Diese Stack-Frames enthalten auch Referenzen auf die lokalen Variablen aller Funktionen, weshalb es hier leicht zu einer zyklischen Referenzierung kommen kann.
Deswegen sollte man `sys.exc_info()[2]` nie dauerhaft speichern und es immer folgendermaßen verwenden:
```python
try:
 tb = sys.exc_info()[2]
 …
finally:
 del tb
```

Noch besser ist es nach Möglichkeit auf `sys.exc_info()[2]` zu verzichten und nur `[0:2]` davon zu speichern, um nicht unbeabsichtigt in das Problem zu laufen.

Mit Python 3 wurde `sys.exc_info()` überarbeitet, denn die Traceback Information ist dort direkt Bestandteil jeder Exception.
Deswegen hat sich dort auch die Bedeutung von `except Exception as ex:` in einem entscheidenden Punkt geändert:
"ex" ist nach dem `except`-Block nicht mehr gesetzt, es passiert also ein implizites `del ex`.
Will man die Exception für später aufheben, muss man sie explizit einer zweiten Variablen zuweisen:
```python
try:
 …
except Exception as ex:
 ex_for_later = ex
code(ex_for_later)
```

Damit riskiert man aber wieder das Problem bzgl. zyklischer Referenzierung wir oben beschrieben, von daher sollte man nach Möglichkeit das nur sehr sparsam einsetzen.
