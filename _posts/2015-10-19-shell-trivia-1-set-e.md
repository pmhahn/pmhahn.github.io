---
title: 'Shell-trivia #1: set -e'
date: '2015-10-19T09:07:03+02:00'
layout: post
categories: shell
excerpt_separator: <!--more-->
---

Kleine Denksportaufgabe für den Morgen:

Was gibt folgendes Skript aus, d.h. was wird durch das `set -e` als Fehler gewertet und führt zum Abbruch des Skripts:

```bash
#!/bin/sh
set -e
! false ; echo 0
! true ; echo 1
true && true ; echo 2
true || true ; echo 3
false && true ; echo 4
false || true ; echo 5
false && false ; echo 6
true || false ; echo 7
true && false ; echo 8
false || false ; echo 9

f () { false; echo 10; }
if f; then echo 11; else echo 12; fi
```

<!--more-->

Hinweise aus [`man 1 bash`](man:bash(1)):

## `-e`
> Exit immediately if a **simple command** (see SHELL GRAMMAR above) exits with a non-zero status.
> The shell **does not exit** if the command that fails is part of the command list immediately following a while or until keyword, part of the test in an if statement, **part of a `&&` or `⎪⎪` list**, or if the command’s return value is being **inverted** via `!`.

## Simple Commands
> A simple command is a sequence of optional variable assignments followed by blank-separated words and re-directions, and **terminated by a control operator**.

## Control operator
> A token that performs a control function.
> It is one of the following symbols:
>     || & && ; ;; ( ) | <newline>
>
> The control operators `&&` and `⎪⎪` denote AND lists and OR lists, respectively.
> An AND list has the form
>
>     command1 && command2
>
> command2 is executed if, and only if, command1 returns an exit status of zero.
>
> An OR list has the form
>
>     command1 ⎪⎪ command2
>
> command2 is executed if and only if command1 returns a non-zero exit status.
> The return status of AND and OR lists is the exit status of the last command executed in the list.

[`man 1 sh`](man:sh(1)) drückt es etwas präziser aus:

## `-e errexit`
> If not interactive, exit immediately if any **untested** command fails.
> The exit status of a command is considered to be explicitly tested if the command is used to control an if, elif, while, or until; or if the command is the **left hand operand** of an `&&` or `||` operator.
