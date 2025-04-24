---
title: 'UNIX 101: file locking'
date: '2013-08-21T05:51:15+02:00'
layout: post
categories: filesystem
---

Ich könnte diese Folge auch „Schlaflos in Oldenburg“ nennen, aber hier ein Beispiel, wie man es nicht macht (in Auszügen):

```python
class Source(object):
  def lock(self, retry=3):
    fd = open(filename, "w")
    try:
      while retry > 0:
        try:
          fcntl.lockf(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
          break
        except IOError:
          retry -= 1
     finally:
       fd.close()

  def unlock(self):
    fd = open(filename, "w")
    try:
      try:
        fcntl.lockf(fd, fcntl.LOCK_UN)
      except IOError:
        pass
    finally:
      fd.close()
```

Nun, sieht doch alles gut aus, selbst an eine saubere Ausnahmebehandlung wurde gedacht.

Nur leider funktioniert das nicht so;
das Problem ist das `fd.close()´:
UNIX assoziiert ein File-Locking mit einer **geöffneten** Datei und nicht mit dem Namen der Datei, d.h. durch das Schließen der Datei wird auch das Locking wieder freigegeben.

Damit das ganze funktioniert, muß man die Datei also geöffnet halten und erst am Ende schließen.
Vereinfacht kann man sich das `unlock()` also sparen und die Datei einfach per `close()` schließen.

Das `fcntl.LOCK_UN` ist für den Fall interessant, wenn man einzelne Bereiche einer Datei sperren und wieder freigeben möchte, z.B. weil man in einer Datei Records fester Größe hat und gezielt einige exklusiv bearbeiten möchte.
Durch kooperatives Locking können dann nebenläufige Prozesse dafür sorgen, daß sie sich nicht gegenseitig auf die Füße treten.

Und um noch die Frage zu beantworten, warum UNIX sich die Locking-Information nur für geöffnete Dateien merkt:
Ganze einfach, um die Ressourcen wieder freigeben zu können.
Solange die Datei sowieso geöffnet ist, müssen dazu im Kern sowieso verschiedene Datenstrukturen gehalten werden, wie Inode-Information, Dateipositionen, etc.
Dort wird auch die Locking-Information abgelegt (siehe `/proc/locks`).
Wenn nun die Datei geschlossen wird, wird auch die zugehörige Locking-Information aus dem Hauptspeicher gelöscht.
Würde man das nur am Dateinamen festmachen, dann hätte man später beim Freigeben der Ressourcen das Problem festzustellen, wann diese nicht mehr benötig wird.

Oder noch Schlimmer:
böse User könnten auf die Idee kommen und alle Dateien zu locken, auf die sie Schreibzugriff haben.
Das würde unbegrenzt viel Speicherplatz **im Kern** verlangen;
so ist das ganze beschränkt auf die Anzahl der Dateien, die ein Benutzer maximal gleichzeitig geöffnet haben kann (`ulimit -m` bzw. `ulimit -x`).

Oder noch Schlimmer:
die Informationen würden persistent im Dateisystem gespeichert werden und sogar einen Neustart überleben …

{% include abbreviations.md %}
