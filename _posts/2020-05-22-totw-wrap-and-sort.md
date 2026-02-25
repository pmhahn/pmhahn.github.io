---
title: 'TotW: wrap-and-sort'
date: '2020-05-22T11:03:18+02:00'
layout: post
categories: debian
tags: totw
excerpt_separator: <!--more-->
---

Q: Gibt es eine empfohlene Schreibweise für die Einträge in `debian/*`-Dateien?

A: Ja, `wrap-and-sort -ast debian/control`

<!--more-->

Leider werden neue Einträge meistens hinten an die _Debian Control file_ bzw. sog. _Debian Maintainer Scripte_ und `debhelper`-Konfigurationsdateien angehängt, was es im Lauf der Zeit sehr unübersichtlich macht.
Um wieder Ordnung in die Dateien zu bringen gibt es [wrap-and-sort](man:wrap-and-sort(1)), das u.a.

- die Dateien in `debian/*.install` u.s.w. sortiert
- die Paket-Abhängigkeiten in `debian/control` sortiert

Die einheitliche Schreibweise mit `--wrap-always --short-indent --trailing-comma` sorgt zudem dafür, das Änderungen dann häufig nur jeweils eine Zeile betreffen, was die `git`-Diffs entsprechend klein und übersichtlich hält.

Außer man schafft es so wie ich und macht sich selbst zum Opfer seiner eigenen [Cleverness](https://github.com/univention/univention-corporate-server/commit/d76f9059ef755b50ad3d90cafffd3739d4a72313) 😉

{% include abbreviations.md %}
