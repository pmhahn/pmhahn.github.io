---
title: 'Pakete aus verschiedenen Repositories'
date: '2011-05-23T12:48:22+02:00'
layout: post
categories: debian UCS
---

Zum Testen des Univention-Updaters wird dieser oft auf einem alten UCS-System installiert und dann damit ein Update auf die aktuellste Version durchgeführt. Dabei mischt man Pakete aus dem offiziellen UCS-Depot mit neueren Paketen von omar. Um sicherzustellen, daß man nur den neueren Updater und nicht auch weitere Pakete von omar verwendet, kann man apt-Pinnging verwenden:

```
#/etc/apt/sources.list:
deb http://omar.knut.univention.de/build2/ ucs_2.4-0-ucs2.4-3/all/
deb http://omar.knut.univention.de/build2/ ucs_2.4-0-ucs2.4-3/$(ARCH)/
```

```
#/etc/apt/preferences
Package: *
Pin: origin omar.knut.univention.de
Pin-Priority: 499

Package: univention-updater
Pin: origin omar.knut.univention.de
Pin-Priority: 501
```

Da unsere Repositories keine `Release`-Datei haben, kann man hier nur `origin` nutzen und sich auf alle Pakete beziehen, die von einem bestimmten Server kommen. Mit den Informationen in den `Release`-Dateien ließen sich auch noch verschiedenen Depots auf dem selben Rechner unterschieden. Mehr dazu in der Manual-Page [`apt_preferences`](http://wiki.debian.org/AptPreferences).

PS: In `aptitude` kann man übrigens auch in der Auflistung eines Pakets gezielt eine der verfügbaren Versionen installieren.
