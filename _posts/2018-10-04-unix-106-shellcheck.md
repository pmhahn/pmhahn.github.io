---
title: 'UNIX 106: shellcheck'
date: '2018-10-04T11:25:27+02:00'
layout: post
categories: shell
tags: totw
---

Kennt ihr schon [shellcheck](https://github.com/koalaman/shellcheck)?

Ist sozusagen [flake8](http://flake8.pycqa.org/en/latest/) für Shell-Skripte.

Man sollte allerdings nicht alles auf die Goldwaage legen, was es anmeckert, denn unser Konstrukt `eval "$(ucr shell)"` führt zu vielen nicht deklarierten Variablen.
Aber es hilft dabei, typische Fehler in Shell-Skripten zu vermeiden, insbesondere bei Quoting.

Und aus aktuellem Anlass: [checkbashisms](man:checkbashisms(1)) aus dem Debian-Paket `devscripts` leistet auch hilfreiche Dienste.

{% include abbreviations.md %}
