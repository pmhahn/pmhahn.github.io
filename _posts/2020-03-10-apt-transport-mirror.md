---
title: apt-transport-mirror
date: '2020-03-10T15:51:06+01:00'
layout: post
categories: debian
---

Q: Kennt ihr `apt-transport-mirror`?

Kennt ihr auch das Problem, dass ihr Debian- (oder UCS-)Systeme habt, die aber zwischen verschiedenen Netzen unterwegs sind (z.B. mein Notebook) und man™ eigentlich je nach Netzwerkumgebung einen anderen Depot-Server für die Debian-Pakete verwenden will?
Wenn ich in Bremen bin, möchte ich z.B. unseren _Internen Debian Mirror_ verwenden, weshalb meine `/etc/apt/sources.list` dann so aussieht:

```
deb http://debian.knut.univention.de/debian/ buster main contrib non-free
```

Wenn ich aber von extern arbeite, möchte ich den [Geo-Location-Dienst deb.debian.org](http://deb.debian.org/) verwenden, um den Netz-topologisch nächsten Spiegelserver zu benutzen:

```
deb http://deb.debian.org/debian/ buster main contrib non-free
```

Bisher habe ich dafür regelmäßig meine `/etc/apt/sources.list` angepasst und jeweils den lokalsten Server eingetragen.
Das hat aber den Nachteil, dass dann jedesmal alle `Packages`-, `Sources`-, `Release*`-, `Index`– und anderen Dateien heruntergeladen werden müssen, denn der Server-Name ist Bestandteil des Cache-Dateien unterhalb von `/var/lib/apt/lists/`.

Besser geht es mit [apt-transport-mirror](man:apt-transport-mirror(1)), das es auch schon mindestens für Debian-Stretch gibt.
Über eine zusätzliche Text-Datei deklariert man eine Liste von Spiegel-Servern, die identischen Inhalt anbieten:

```bash
cat >/etc/apt/mirrorlist.txt <<__EOF__
http://debian.knut.univention.de/debian/
http://deb.debian.org/debian/
__EOF__
```

Anschließend kann man diese Liste statt des Server-Namens in der `/etc/apt/sources.list` verwenden:

```
deb mirror+file:/etc/apt/mirrorlist.txt buster main
deb mirror+file:/etc/apt/mirrorlist.txt buster-updates main
```

PS: Die `sources.list` unterstützt übrigend eine 2. Syntax nach nach Deb822-Stiel:

```
Types: deb deb-src
URIs: mirror+file:/etc/apt/mirrorlist.txt
Suites: buster buster-updates
Components: main contrib non-free

Types: deb deb-src
URIs: http://security.debian.org
Suites: buster/updates
Components: main contrib non-free
```

Diese Schreibweise ist deutlich kompakter, vor allem wenn man viele *Suites* und *Componenten* einbinden will, doppelt wenn man auch die Quellen benötigt.
