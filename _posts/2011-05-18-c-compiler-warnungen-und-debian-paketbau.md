---
title: 'C-Compiler Warnungen und Debian Paketbau'
date: '2011-05-18T11:17:42+02:00'
excerpt: 'dpkg-buildflags erweitern.'
layout: post
categories: c debian
---

Um beim testweisen Paketbau dem C-Compiler noch ein paar zusätzliche Flags (z.B. `-Wall` zum anzeigen aller Warnungen oder `-Werror`, um daraus sogar Fehler zu machen, die zum Abbrechen des Übersetzungsvorgans führen), kann man diese per `export DEB_CFLAGS_APPEND=-Wall` für nachfolgende Aufrufe von `debuild` bzw. `dpkg-buildpackage` hinzufügen. Die Manual-Page [dpkg-buildflags(1)](man:dpkg-buildflags(1)) verrät weitere Einzelheiten.

{% include abbreviations.md %}
