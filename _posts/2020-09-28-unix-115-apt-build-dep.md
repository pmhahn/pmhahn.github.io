---
title: 'UNIX 115: apt build-dep .'
date: '2020-09-28T06:46:22+02:00'
layout: post
categories: debian
---

Q: Wie installiere ich die für den Bau des Debian-Pakets im aktuellen Arbeitsverzeichnis notwendigen Pakete?

A: `apt build-dep .` interaktiv bzw. `apt-get -q --assume-yes build-dep .` in Skripten

Damit müsst ihr nicht mal mehr das Paket [build-essential](https://packages.debian.org/search?keywords=build-essential&searchon=sourcenames&suite=all&section=all) von Hand vorab installieren, weil das auch automatisch mit installiert wird.

Natürlich könnte ich auch weiterhin die [benötigten Pakete](https://www.debian.org/doc/debian-policy/ch-relationships.html) aus der Datei `debian/control` im Abschnitt `Source` unter `Build-Depends{,-Arch,-Indep}` und die Negativlisten unter `Build-Conflicts{,-Arch,-Indep}` heraussuchen und von Hand per `apt-get install --no-install-recommends …` installieren.
Oder ihr verwendet wenigstens `dpkg-checkbuilddeps` für das heraussuchen der fehlenden Pakete.

Damit braucht man auch kein `/usr/lib/pbuilder/pbuilder-satisfydepends` mehr, wovon es 5 Varianten gibt.
