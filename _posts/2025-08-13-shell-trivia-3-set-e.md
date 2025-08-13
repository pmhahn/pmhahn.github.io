---
title: 'Shell-trivia #3: set -e'
date: '2025-08-13T08:39:00+02:00'
layout: post
categories: shell
excerpt_separator: <!--more-->
---

Es gab bereits zwei Blog-Eintrag [Shell-trivia #1]({% post_url 2015-10-19-shell-trivia-1-set-e %}) und [Shell-trivia #2](% post_url 2019-04-15-shell-trivia-2-set-e %) zum Thema `set -e`, aber mein Kollege @nschier hat mich heute Morgen mit einer weiteren Shell-Absurdität überrascht:

```bash
#!/bin/sh
set -e
date && false && true
date
```

Wie häufig wird `date` ausgeführt?

<!--more-->

Wie üblich muss man die [Manual-Page von bash](https://manpages.debian.org/stretch/bash/bash.1.en.html#Shell_Function_Definitions) sehr genau lesen:

> The ERR trap is ***not* executed if the failed command is … part of a command executed in a && or || list except the command **following the final** && or ||.

Die korrekte Antwort lautet also: 2

Beim `date && false && true` Endet die Ausführung nach dem `false` und der Exit-Code ist 1:
Das nachfolgende `&& …` wird nicht mehr ausgeführt.
Da das `false` aber dadurch nicht der letzte Befehl ist, bricht `set -e` nicht ab und das 2. `date` wird trotzdem ausgeführt.

Um es mit den Worten von Gerd A. zu sagen:
> Jedes gute Shell-Script fängt mit #!/usr/bin/perl an.

Naja, das war 1995 und ich würde `perl` durch `python` ersetzten 😉
