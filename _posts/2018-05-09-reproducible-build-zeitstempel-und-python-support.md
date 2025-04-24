---
title: 'Reproducible Build, Zeitstempel und python-support'
date: '2018-05-09T12:03:46+02:00'
layout: post
categories: debian python
---

Debian ist ein treibende Kraft für [reproduzierbaren Paketbau](https://wiki.debian.org/ReproducibleBuilds):
Ziel ist es, dass jeder ein Quellpaket erneut bauen kann und dann bei gleichen Vorbedingungen (Compiler-/Library-Versionen) exakt die selben Binärpakete enthält.
Das ist wichtig für die Sicherheit, um garantieren zu können, dass weder auf dem Rechner des Entwicklers noch auf den Servern von Debian manipulierte Software den Paketbau beeinflussen.

Ein Problem dabei ist:
Welchen Zeitstempel sollen die Dateien im erzeugten Paket haben?
Debian verwendet den Zeitstempel des letzten Eintrags aus `debian/changelog`.
Dass kann zu Problemen führen, wenn man ein Paket mehrfach neu baut und immer wieder installiert.
Z.B. vergleicht `update-python-modules` die Zeitstempel der Python-Module `*.py` mit den kompilierten Dateien `*.pyc` und kompiliert diese dann nicht neu, weil das Kompilat vom letzten mal ja schon neuer ist als die scheinbar immer noch unveränderten Python-Dateien.
In diesem Fall hilft ein `sudo update-python-modules -f`, was eine Neukompilierung **aller** Python-Module erzwingt.
