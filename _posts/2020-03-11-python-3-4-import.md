---
title: 'Python 3.4: import'
date: '2020-03-11T09:41:35+01:00'
layout: post
categories: python
---

## relativ vs. absolut

Ursprünglich implementiert Python **relative** imports, d.h. schreibt man im Module `foo` ein `import bar`, such Python 2 zunächst nach dem Modul `foo.bar`.
Findet es diese nicht, sucht es dann nach `bar`.

Python 3 verwendet standardmäßig **absolute** imports, was man auch schon in Python 2 mit einem `from __future__ import absolute_import` erreichen kann.

Module können dann so importiert werden:
```python
# absoluter import
import abso.lut
from abso import lut

# relativer import
import .rela.tiv
from .rela import tiv

# oder auch so
import ..siblling.child
from ..sibling import child
```

Problematisch dabei ist, dass man ab dann diese Module nicht mehr direkt aufrufen kann, also so etwas wie: `python2 /usr/lib/python2.7/dist-packages/abso/lut.py`.
Denn das Modul weiß nicht, das es eigentlich `sys.modules["abso.lut"]` ist und kann deswegen die relativen Referenzen nicht auflösen.
Ein `python2 -m abso.lut` dagegen funktioniert, weil Python das Modul dann eben über diesen Namen importiert und damit dann auch relative Imports auflösen kann.

## Pfade dist

Mit der Umstellung von `python-support` auf `dh-python2` haben wir gerade mühevoll überall `/usr/share/pyshared/` bzw. `/usr/lib/pymodules/python2.7/` durch `/usr/lib/python2.7/dist-packages/` ersetzt, mit Python 3 gibt es natürlich einen neue Pfade:
Unter `/usr/lib/python3/dist-packages/` landen unsere Pakete, aber es gibt eben auch noch `/usr/lib/python3**.X**/dist-packages/` für jede Version des Python 3 Interpreters.
Mit UCS-4 (= Debian Stretch) haben wir **Python 3.5**, UCS-5 (= Debian Buster) aber **Python 3.7**. Zum Glück kann uns das derzeit aber egal sein.
Trotzdem sollten wir es vermeiden, diese Pfade überall hart zu kodieren, wie es derzeit aber leider in UCS manchmal noch der Fall ist:
```bash
# Bad
/usr/bin/python2.7 /usr/lib/pymodules/python2.7/univention/s4connector/s4/main.py
# Good
python2.7 -m univention.s4connector.s4.main
```

Aber auch hier steckt der Teufel im Detail und man muss jeden Fall genau anschauen, denn manchmal funktioniert es so einfach dann doch nicht:
Wegen relative Imports werden dann plötzlich Module nicht mehr gefunden oder der Code kommt nicht mit dem geänderten `__main__` klar.

## Pfade src

Die Umstellung auf relative Imports wird zusätzlich bei uns in UCS dadurch erschwert, dass wir einen Wildwuchs von **unterschiedlichen Strukturen** in unseren Qullpaketen haben:

1. Die **installierten** Module von `univention.lib` liegen im Quellcode aber unter `python/`.
2. Beim Listener dagegen gibt es im Quelllcode bereits das Verzeichnis `python/`, unter dem dann die Dateien „schon an der richtigen Stelle“ liegen.
3. Bei `management/univention-management-console` heißt das Verzeichnis dagegen `src/`.
4. Bei `base/univention-ipcalc` liegt das `univention/`-Verzeichnis direkt unterhalb der Wurzel.
5. Bei `base/univention-python` auch direkt unterhalb der Wurzel, werden aber nach `univention` installiert.

Diese unterschiedlichen Pfade haben schon bei der Generierung der [Python API Dokumentation](https://docs.software-univention.de/ucs-python-api/) zu Problemen geführt.
Eine Vereinheitlichung halte ich für sehr sinnvoll, den mit der aktuellen Struktur ist ein Aufruf der Module über `python -m` nicht möglich.

### Current best practice

> Unterhalb eines `src/`-Verzeichnisses liegen die Python-Dateien an der Stelle, an der sie dann später auch installiert werden.
> Das Quellcodepaket beinhaltet enthält in allen Verzeichnissen eine `__init__.py` mit dem [`pkgutil.extend_path`](https://docs.python.org/3/library/pkgutil.html#pkgutil.extend_path)-Fraagment, um zu gewährleisten, das die die Pakete gegenseitig ergänzen können.
> Diese zusätzlichen `__init__.py`-Dateien werden aber nicht installiert und sind damit nicht Bestandteil des Binärpakete.

Grund für das zusätzliche `src/`-Unterverzeichnis ist, dass standardmäßig das aktuelle Arbeitsverzeichnis `.` in `sys.path` an erster Stelle eingefügt wird.
Befindet man sich also im Wurzelverzeichnis eines Quellpakets, würden die Python-Module darin also direkt verwendet werden, wenn sie direkt darin liegen würden.
Das ist nicht immer sinnvoll und führt unfreiwillig zu Problemen.
Deswegen lautet die Empfehlung, alles unterhalb eines Unterverzeichnisses `src/` abzulegen;
man kann dann dieses Verzeichnis entweder explizit über `PYTHONPATH=src` aufnehmen oder wechselt explizit in das `src/`-Verzeichnis, damit dieses dann über den `.`-Mechanismus aufgenommen wird.

{% include abbreviations.md %}
