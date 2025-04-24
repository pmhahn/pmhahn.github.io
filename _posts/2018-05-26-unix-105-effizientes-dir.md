---
title: 'UNIX 105: Effizientes dir'
date: '2018-05-26T07:01:24+02:00'
layout: post
categories: filesystem
---

Viele von euch werden schon mal ein `ls` auf **omar** in `/var/univention/buildsystem2/apt/ucs_4.3-0/all/` oder einem ähnlichen Verzeichnis ausgeführt und sich gewundert haben, warum das so lange dauert.

## GLIBC

Fangen wir mal damit an, uns das Verzeichnis genauer anzuschauen:

```console
# ls -1 /var/univention/buildsystem2/apt/ucs_4.3-0/all | wc -l
21174
```

In dem Verzeichnis liegen also 21k Dateien. Das `ls` macht folgendes:
```c
fd = opendir(…)
while(…) {
 ent = readdir(fd);
 printf(…);
}
closedir(fd)
```

D.h. für jede der 21k Dateien wird die glibc-Funktion `readdir()` aufgerufen.
Zum Glück führt nicht jeder Aufruf auch zu einem Linux-Systemaufruf, denn `getdents64()` verwendet einen internen Puffer und liefert mehrere aufeinanderfolgende Verzeichniseinträge pro Aufruf zurück.
Allerdings ist dieser Puffer nur **32 KiB** groß.
Schaut man sich das Verzeichnis nochmals an, sieht man folgendes:

```console
# ls -dgGh /var/univention/buildsystem2/apt/ucs_4.3-0/all
drwxrwsr-x 1,5M Mär 14 10:41 /var/univention/buildsystem2/apt/ucs_4.3-0/all
           ^^^^
```

Der Verzeichniseintrag belegt 1.5 MiB.
Das entspricht zwar nicht ganz der zu übertragenden Datenmenge, denn selbst wenn man später Dateien aus dem Verzeichnis löscht, wird der Platz für den Eintrag nicht wieder freigegeben;
der Platz wird später für neue Einträge wiederverwendet.
Selbst wen man das rausrechnet bleiben noch ca. 40 Systemaufrufe für `getdents64()` übrig.

Eine nette Anekdote dazu ist [You can list a directory containing 8 million files! But not with ls](http://be-n.com/spw/you-can-list-a-million-files-in-a-directory-but-not-with-ls.html).

Noch schlimmer wird es, wenn man `ls -l` aufruft. Denn dann muss auch noch für jeden Namen ein `lstat64()`-Systemaufruf getätigt werden, um für jede Datei einzeln den Status (Größe, Besitzer, Rechte, …) abzufrgaen.

Historisch gesehen musste man das sogar machen um festzustellen, ob es sich bei dem Eintrag um eine Datei, ein Verzeichnis oder einen symbolischen Link handelt.
Einige Dateisystemen (ext4, btrfs, …) speichern diese Information aber nicht nur in der inode (das ist das, was man mit dem `stat()`-Aufruf abfragt), sonder zusätzlich auch im Verzeichniseintrag selbst (das ist das, was man mit `getdents()` abfragt).
Das nutzt z.B. `ls` dafür, um Verzeichnisse anders anzuzeigen als Dateien; es spart sich dadurch 21k Systemaufrufe.

Kommt es auf die Reihenfolge nicht an, so kann man `ls -U` verwenden, was einem die Zeit für das Sortieren der Einträge spart.
Ist zwar nur `O(N*log(n))`, kostet aber gerade bei großen Verzeichnissen eben doch Zeit.

Auch ein beliebtes Performance-Problem ist es, die Ausgabe von `listdir()` zu sortieren und dann `stat()` aufzurufen.
Das führt insbesondere bei rotierenden Festplatten gerne dazu, dass durch das Sortieren anhand des Namens die inode-Nummern dann wild hin und her springen, was zu längeren Suchzeiten führt.
Deswegen lautet die Empfehlung, die `stat()`-Aufrufe in der Reihenfolge zu tätigen, wie die Einträge von `listdir()` geliefert werden – sofern man denn die `stat()`-Aufrufe überhaupt benötigt.

Das ist auch ein Grund, warum man in der Shell an dieser Stelle keine Wildcards verwenden sollte:
```bash
ls -1d *
```

1. Hier expandier zunächst die **Shell** das `*` und liest das Verzeichnis aus.
2. Für jeden gefundenen Eintrag ruft sie `stat()` auf.
3. Diese (lange) Liste von Argumenten wird an `ls` übergeben, was dann **abermals** `stat()` für jedes Argument aufrufen.

## Python

In Python hat man das Problem, das man durch die Platformunabhängigkeit noch ein Stück weiter weg ist von einer effizienten Implementierung:

- `os.listdir()` verwendet `readdir()` und damit auch nur den kleinen 32k Puffer.
- `os.walk()` macht für jeden Verzeichniseintrag immer einen `stat()`-Aufruf, um Verzeichnisse für die Rekusrion zu erkennen.

Zumindest gegen letzteres gibt es deswegen [PEP-0471](https://www.python.org/dev/peps/pep-0471/), was als Nebenprodukt eine intelligentere Implementierung für `os.walk()` nachliefert:
Statt für jeden Verzeichniseintrag jedesmal ein `stat()` aufzurufen, verlässt es sich auf den sowieso von `readdir()` gelieferten Dateityp – sofern das Dateisystem eben diesen liefert.
Damit ist diese Implementierung in vielen Fällen spürbar schneller.

Leider gibt es auch bei dieser Implementierung keine Möglichkeit, die Puffergröße anzupassen.
Dazu muss man dann selber die Systemaufrufe mit [ctypes](https://stackoverflow.com/questions/37032203/make-syscall-in-python) machen.

## Linux-Kernel

Nach dem ersten `listdir()`-Aufruf sind die nachfolgenden Aufrufe für das selbe Verzeichnis i.d.R. deutlich schneller.
Das liegt daran, dass der Linux Kernel die `dirent`-Einträge im sog. **dcache** zwischenspeichert.
Informationen zu diesem Cache kann man in `/proc/slabinfo` unter **dentry** auslesen:

```console
$ grep -e dentry -e ^# /proc/slabinfo
# name <active_objs> <num_objs> <objsize> <objperslab> <pagesperslab> : tunables <limit> <batchcount> <sharedfactor> : slabdata <active_slabs> <num_slabs> <sharedavail>
dentry 763495 765177 192 21 1 : tunables 120 60 8 : slabdata 6437 36437 0
```

Ebenfalls bemerkenswert ist es, dass ein direkter `open()`-Aufruf auf ein Unterverzeichnis nicht die Performance-Probleme von `listdir()` zeigt:
Beim Auflösen eines Pfads wir jede Verzeichniskomponente direkt nachgeschlagen.
Das geht z.B. beim Dateisystem **ext4** deutlich schneller als das Auflisten aller Einträge, weil das Dateisystem intern einen [HTree](https://en.wikipedia.org/wiki/HTree) verwendet.
Anhand des Hash-Wertes der Pfadkomponente kann schnell überprüft werden, ob es ein solchen Eintrag gibt.
Ältere Dateisysteme müssen dafür tatsächlich noch über alle Verzeichniseinträge iterieren, um erst im ungünstigsten Fall am Ende zu erkennen, dass es den gewünschten Eintrag nicht gibt.

{% include abbreviations.md %}
