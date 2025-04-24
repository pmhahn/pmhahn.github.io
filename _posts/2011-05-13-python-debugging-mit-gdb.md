---
title: 'Python debugging mit gdb'
date: '2011-05-13T09:02:56+02:00'
layout: post
categories: python
---

Wenn ein Python-Prozeß zu hängen scheint und keinen Mucks mehr von sich gibt, kann `gdb` noch ein Stück weiterhelfen um herauszubekommen, wo der Prozeß hängt.
Unter `/usr/share/doc/python2.?/gdbinit*` gibt es verschiedene Macros, die in `gdb -p "$PID"` per `source /usr/share/doc/python2.?/gdbinit*` eingelesen werden können.
Anschließend liefert ein `thread apply all pystack` einen Strack-Trace aller Threads.
Damit das ganze funktioniert, sollten einige Debug-Pakete installiert sein, mindestens jedoch **python-dbg**, **libc6-dbg**.
<!-- Leider funktioniert das Macro nicht immer, deswegen gibt es im Toolshed das Programm **gdbpystack**, daß das Erzeugen eines Python-Stacktraces vereinfacht. -->

{% include abbreviations.md %}
