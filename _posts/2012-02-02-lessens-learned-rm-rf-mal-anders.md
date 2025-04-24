---
title: 'Lessons learned: &#8222;rm -rf /&#8220; mal anders'
date: '2012-02-02T08:48:14+01:00'
layout: post
categories: linux filesystem shell
---

Der Ein oder Andere wird es vielleicht mitbekommen haben, aber einer unserer Kunden hat Ende Januar die Gefährlichkeit eines `rm -rf /` als Benutzer *root* erfahren (für die nicht-Techniker:
Lösche rekursiv alle Dateien und Verzeichnisse, angefangen beim Wurzelverzeichnis und ohne weiteres nachfragen).
Zum Glück hatte der Kunde ein 18h altes Backup, aber trotzdem ist viel Arbeit über den Jordan gegangen, weil unter anderem auch die ganzen virtuellen Maschinen per NFS unter /nfs/ gemounted waren… die Betonung liegt hier auf „waren“ 🙁
Wäre es an der Stelle nur ein `rm` gewesen, wäre die Sache vermutlich auch noch gut ausgegangen, denn ein `rm -rf /` kann man gefahrlos einfach mal so eingeben:

```console
# rm -rf /
rm: Es ist gefährlich, rekursiv auf „/“ zu arbeiten.
rm: Benutzen Sie --no-preserve-root, um diese Sicherheitsmaßnahme zu umgehen.
```

Denn um diesen „shoot yourself in the foot“ hat sich zum Glück schon mal [Jemand](http://blogs.oracle.com/jbeck/entry/rm_rf_protection ""rm -rf /" protection ") gekümmert.
Dumm war nämlich nur, das es bei dem Kunden ein `rsync -a --delete /tmp/ /` war, was diese Sicherungsebene **nicht** hat.
(für die nicht-Techniker:
Synchronisiere rekursiv den Inhalt der Verzeichnisses `/tmp/` in das Wurzelverzeichnis und **lösche alle Dateien, die nicht im Quellverzeichnis vorhanden sind**.) Deswegen sollte man doch zumindest über die Option `--max-delete`von `rsync` nachdenken.

Und das mir bitte niemand `rsync -a --delete $source $target/` schreibt!
Erstens fehlt da definitiv das ordnungsgemäße Quoting, und zweitens:
Macht euch mal Gedanken was passiert, wenn die Variable `target` dann doch plötzlich leer ist… das war nämlich das Problem des Kunden 🙁

{% include abbreviations.md %}
