---
title: 'UNIX 111: envsubst'
date: '2020-03-10T14:55:35+01:00'
layout: post
categories: shell UCS
---

UCR implementiert einen Template-Mechanismus:
Er ersetzt Platzhalter in der Vorlage durch die konfigurierten Werte (oder führt passenden Python-Code aus).
```bash
echo '@%@hostname@%@' | ucr filter
```
In vielen anderen Situationen wird gerne `sed` (oder ähnliches) genutzt, um Platzhalter in anderen Dateien zu ersetzten:
```bash
echo '@XXX@' | sed -e "s/@XXX@/$(hostname)/g"
```
Bei dieser Art der Ersetzung übersieht man gerne, dass man dort eigentlich die Werte passend escapen muss, denn ein `/` würde hier den sed-Befehl ändern.

Eine Alternative ist [`envsubst`](man:envsubst(1)) aus den `gettext-base`, mit denen man über die Prozess-Umgebungsvariablen eine Ersetzung durchführen kann:
```bash
echo '${USER}' | envsubst
```
Normalerweise ersetzt `envsubst` alle Referenzen auf Umgebungsvariablen.
Möchte man nur einige ersetzte, so kann man die zu ersetzenden Variablen auch explizit angeben:
```bash
printf '$A\n${B}\n' | env A=aaa B=bbb envsubst '$B'
```

{% include abbreviations.md %}
