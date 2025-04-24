---
title: 'Shell-trivia #2: set -e'
date: '2019-04-15T10:47:53+02:00'
layout: post
categories: shell
---

Es gab bereits einen Blog-Eintrag [Shell-trivia #1: set -e]({% post_url 2015-10-19-shell-trivia-1-set-e %}), aus gegebenem Anlass hier die Fortsetzung:

Per `set -e` kann man die Shell veranlassen, ein Skript abzubrechen, sobald eins der Kommandos fehlschlägt. Leider steckt der Teufel hier im Detail.

Betrachten wir folgendes Beispiel:
```bash
#!/bin/sh
my_func () {
 false # fehlschlagendes Kommando
 : # ein nachfolgendes Kommando
}
if my_func
then
 echo „Erfolg“
else
 echo „Fehlschlag“
fi
```

Meine Erwartung war **Fehlschlag**, aber man bekommt **Erfolg**.
Warum?

Wie üblich muss man die [Manual-Page von bash](https://manpages.debian.org/stretch/bash/bash.1.en.html#Shell_Function_Definitions) sehr genau lesen:

> set -e: Exit immediately if a **simple command** exits with a non-zero status. The shell does **not** exit if the command that fails is part of the command list immediately following a `while` or `until` keyword, part of the test in an `if` statement, part of a `&&` or `⎪⎪` list, or if the command’s return value is being inverted via `!`.

An [anderer Stelle](https://manpages.debian.org/stretch/bash/bash.1.en.html#Compound_Commands) liest man noch folgendes:

> A shell function is an object that is called like a **simple command** and executes a **compound command** with a new set of positional parameters.

Merke also:

`set -e` führt **nicht** zum Abbruch von eines Compound Commands, sobald dieses bedingt (durch `if`, `while`, `until`, `&&`, `||`, etc.) ausgeführt wird.

PS: Der Rückgabewert eines Funktionsaufrufs ergibt sich aus dem Exit-Status des letzten Befehls im Coumpound Command, d.h. oben `:`, was ähnlich wie `true` immer gelingt und `0` zurück liefert.

{% include abbreviations.md %}
