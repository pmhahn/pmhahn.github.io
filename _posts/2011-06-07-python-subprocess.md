---
title: 'Python subprocess'
date: '2011-06-07T17:03:06+02:00'
layout: post
categories: python
---

Das Python `subprocess` Modul hat so seine Tücken:

1. Bei Verwendung von `subprocess.PIPE` muss unbedingt darauf geachtet werden, das Pipes unter Linux nur eine endliche Größe (64 KiB) haben.
   Wenn der Kindprozeß als Filter (STDIN und STDOUT bzw. STDERR sind Pipes zum Vaterprozeß) verwendet wird, **muß** unbedingt `subprocess.Popen.communicate(input)` verwendet werden, da die Prozesse ansonsten blockieren:

```python
#!/usr/bin/python
from subprocess import PIPE, Popen
p = Popen(["dd", "bs=1"], stdin=PIPE, stdout=PIPE)
p.stdin.write("x" * (2 * (64 << 10) + 1))
```

Nach der letzten Zeile blockiert das ganze, da beide Pipes gefüllt sind und für das nächste Zeichen dann nirgendwo mehr Platz ist.
Richtig ist es dann so:

```python
#!/usr/bin/python
from subprocess import PIPE, Popen
p = Popen(["dd", "bs=1"], stdin=PIPE, stdout=PIPE)
stdout, stderr = p.communicate("x" * (2 * (64 << 10) + 1))
```

Python nutzt dazu intern Threads, die im Hintergrund die Daten parallel lesen und schreiben, wodurch das voll-laufen der Pipes vermieden wird.

2. `subprocess.Popen()` startet den Prozeß augenblicklich und kann ohne `wait()` auch dazu genutzt werden, Kindprozesse in den Hintergrund zu forken.
   Lange laufende Vaterprozesse sollten allerdings eine Referenz auf sie behalten und von Zeit zu Zeit per `poll()` prüfen, ob sich diese Prozesse inzwischen beendet haben und beerdigt werden können.
   Ansonsten bleiben diese als sog. Zombie-Prozesse solange bestehen, bis sich irgendwann auch mal der Vaterprozeß beendet, wodurch die toten Kinder dann dem init-Prozeß vermacht werden, der sie dann endgültig beerdigt.

```python
#!/usr/bin/python
from time import sleep
from subprocess import Popen
children = [
    Popen(["sleep", "10"]),
]
while children:
  sleep(1)  # do some real work
  for p in children:
    if p.poll() is not None:
      children.remove(p)
```

Zombie-Prozesse sind an sich nichts schlimmes und belegen nur noch wenige Ressourcen im Betriebssystemkern.
Sie zeugen allerdings von einem unsauberen Vaterprozeß, denn dessen Aufgabe ist es, auch für seine Kinder bis zum bitteren Ende zu sorgen.
Ein schlechtes Beispiel ist hier die `run()`-Methode des "UCS Listener", die nämlich nur einen Kindprozeß forked, diesen dann aber vergisst, was dann zu solchen Dingen wie in [Bug #21363](https://forge.univention.org/bugzilla/show_bug.cgi?id=21363 "Zombie Prozesse wenn Listner UCR Script triggert durch das Setzen von UCR Variablen") führt.

{% include abbreviations.md %}
