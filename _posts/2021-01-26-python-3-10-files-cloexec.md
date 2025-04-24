---
title: 'Python 3.10: files CLOEXEC'
date: '2021-01-26T11:12:11+01:00'
layout: post
categories: python
---

In Python 3 gabt es mehrere wichtige Änderung bezüglich Dateien und Prozessen:
Um aus [open()](https://docs.python.org/3.7/library/functions.html#open) zu zitieren:

> The newly created file is [non-inheritable](https://docs.python.org/3.7/library/os.html#fd-inheritance).

Bisher wurden Dateien normal geöffnet, d.h. Kindprozesse haben diese geöffneten Dateideskriptoren geerbt und sich diese mit den Eltern geteilt.
Das ist das Standardverhalten, was notwendig ist, damit Prozesse gemeinsam an Dateien arbeiten können:
```bash
(
 echo "Anfang"
 echo "Ende"
) >/tmp/Datei
```

Hier wird die Datei von der Vater-Shell geöffnet und an die beiden Kindprozesse vererbt.
Diese schreiben nacheinander in die Datei und aktualisieren dabei nacheinander die Position innerhalb der Datei, so dass sie ihre Ausgaben aneinander hängen.

Das Vererben der offenen Dateien führt mitunter aber zu Problemen, wenn dies unbeabsichtigt geschieht:

- Wir hatten mal ein [Problem mit Apache](https://forge.univention.org/bugzilla/show_bug.cgi?id=37952), was mit [geerbten `pipe()`s]({% post_url 2013-08-21-unix-102-pipes %}) zu tun hatte.
- [LVM](https://forge.univention.org/bugzilla/show_bug.cgi?id=30550) beschwert sich regelmäßig

Deshalb gibt es die Möglichkeit, geöffnete Dateien per [`fcntl(…, FD_CLOEXEC)`](man:fcntl(2)) so zu markieren, das diese nach einem [`execve(2)`](man:execve(2)) automatisch geschlossen werden.
Nachteilig daran ist, dass man es explizit machen muss und dass es Race-Conditions gibt.
Deshalb gibt es bei vielen System-Aufrufen wie [`open(2)`](man:open(2)) und [`pipe(2)`](man:pipe(2)) inzwischen Varianten, bei denen man direkt `O_CLOEXEC` setzten kann.

Das macht Python 3.4 nun auch standardmäßig, was ggf. zu einem geänderten Verhalten führt, insbesondere bei der Verwendung von [subprocess](https://docs.python.org/3.7/library/subprocess.html):

- Bisher musste man `close_fds=True` explizit nutzen, seit Python 3.2 es standardmäßig aktiviert
- Bewusst weiterzureichende Datei-Deskriptoren können und müssen per `pass_fds` durchgereicht werden
