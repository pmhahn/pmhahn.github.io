---
title: 'Effizientes Kopieren von großen Dateien'
date: '2011-06-06T11:57:07+02:00'
layout: post
categories: linux filesystem
---

Mit Virtualisierung trifft man öfters auf mehrere Gigabyte große Image-Dateien, die von Zeit zu Zeit auch mal kopiert werden müssen. Klassischerweise passiert das mit `cp` oder `dd bs=1G`. Beide Varianten lesen die Daten häppchenweise per `read()` in den Hauptspeicher, um ihn anschließend per `write()` in eine neue Datei zu schreiben. Die `dd`-Varianta hat hier den Vorteil, daß man durch Angab der Blockgröße größere Datenblöcke am Stück einlesen kann, was vor allem die Anzahl der Lese-/Schreibkopfpositionierungen der Festplatte reduziert, was zu erheblichen Geschwindigkeitsvorteilen führen kann. `cp` verwendet normalerweise 32 KiB-Blöcke, `shutil.copy2()` aus Python nur 16 KiB-Blöcke (und kopiert nicht alle Berechtigungen!)

Das es effizienter geht zeiht **BtrFS** mit der Möglichkeit, Dateien per `cp –reflink` zu klonen:

```console
# ls -li orig.tar.gz
257 -rw-r–r– 1 root root 82167227 6. Jun 11:14 orig.tar.gz
# cp –reflink orig.tar.gz copy
# ls -li orig.tar.gz copy
257 -rw-r–r– 1 root root 82167227 6. Jun 11:14 orig.tar.gz
258 -rw-r–r– 1 root root 82167227 6. Jun 11:14 copy
# md5sum orig.tar.gz copy
290e2524776f33d6ce8c5f9d61545f34 orig.tar.gz
290e2524776f33d6ce8c5f9d61545f34 copy
# dd if=/dev/zero of=copy bs=1M count=1 conv=notrunc status=noxfer
# ls -li orig.tar.gz copy
257 -rw-r–r– 1 root root 82167227 6. Jun 11:14 orig.tar.gz
258 -rw-r–r– 1 root root 82167227 6. Jun 11:16 copy
# md5sum orig.tar.gz copy
290e2524776f33d6ce8c5f9d61545f34 orig.tar.gz
2636db080b31d5181602d5d092d42ea1 copy
```

Ähnlich zu `ln` wird hier aber nicht einfach ein neuer Hard-Link auf die Inode angelegt, so daß Änderung über *einen Namen* sich auch auf den Inhalt auswirken, der über *den anderen Namen* angesprochen wird. Statt dessen bekommt die Kopie eine eigene Inode, sie sich per Copy-on-write die *Datenblöcke* mit der Ursprungsdatei teilt. Erst durch Modifikation einer der beiden Dateien driften diese auseinander und verwenden bei fortschreitender Modifikation irgendwann getrennte Datenblöcke.
