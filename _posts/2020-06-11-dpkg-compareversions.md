---
title: 'dpkg --compare-versions'
date: '2020-06-11T11:26:58+02:00'
layout: post
categories: debian
excerpt_separator: <!--more-->
---

Q: Wie unterscheide in Upgrades von Neuinstallationen?

A: `dpkg --compare-versions "$2" lt-nl "…"`

<!--more-->

Die [Debian package maintainer scripts](https://www.debian.org/doc/debian-policy/ch-maintainerscripts.html) erhalten mehrere Parameter beim Aufruf:
`"$1"` enthält die Aktion:

| action              | preinst | postinst | prerm | postrm |
| ------------------- | ------- | -------- | ----- | ------ |
| `abort-deconfigure` |         | ☒        |       |        |
| `abort-install`     |         |          |       | ☒      |
| `abort-remove`      |         | ☒        |       |        |
| `abort-upgrade`     | ☒       | ☒        |       | ☒      |
| `configure`         |         | ☒        |       |        |
| `deconfigure`       |         |          | ☒     |        |
| `disappear`         |         |          |       | ☒      |
| `failed-upgrade`    |         |          | ☒     | ☒      |
| `install`           | ☒       |          |       |        |
| `purge`             |         |          |       | ☒      |
| `remove`            |         |          | ☒     | ☒      |
| `triggered`         |         | ☒        |       |        |
| `upgrade`           | ☒       |          | ☒     | ☒      |

Der 2. Parameter `"$2"` enthält meist die Vorgänger bzw. Nachfolger-[Version des Pakets]({% post_url 2020-06-10-debian-versions-schema %}).
Letztere ist für Neuinstallationen leer `''`, für Upgrades aber nicht.

`dpkg --compare-versions` unterstützt extra dafür die `-nl`-Varianten ("not less") der Vergleichsoperatoren `lt` (less-than), `le` (less-equal), `eq` (equal), `ne` (not-equal), `ge` (greater-equal), `gt` (greater-than), die den leeren-Wert bei Neuinstallationen eben anders behandeln:
Normal steht die leere Version `''` immer für die kleinste Version, bei den `-nl`-Varianten dagegen als die größte.

Das folgende Fragment findet man z.B. häufiger in `postinst` Skripten:

```bash
case "$1" in
  configure)
    if dpkg --compare-versions "$2" lt-nl "$FIXED_VERSION"
    then
      echo "Only on upgrade"
    elif dpkg --compare-versions "$2" lt "$FIXED_VERSION"
    then
      echo "On new or upgrade"
    fi
    ;;
esac
```

Das folgende Beispiel zeigt z.B. das unterschiedliche Verhalten, wenn Paketversion 2 installiert wird und ggf. Version 1 vorher installiert war bzw. 2 (oder neuer) bereits installiert ist:

```bash
dpkg --compare-versions ''  lt    2 # 0
dpkg --compare-versions ''  lt-nl 2 # 1
dpkg --compare-versions '1' lt    2 # 0
dpkg --compare-versions '1' lt-nl 2 # 0
dpkg --compare-versions '2' lt    2 # 1
dpkg --compare-versions '2' lt-nl 2 # 1
```

PS: Bitte vermeidet `test … -a …` und `test … -o …` für logische Verknüpfungen, sondern verwendet `[ … ] && [ … ]` bzw. `[ … ] || [ … ]`, da erstere [nicht wohldefiniert sind](https://github.com/koalaman/shellcheck/wiki/SC2166).

<!--
PPS: Hier mein [Vortrag zu den Maintainer-Scripten](https://phahn.gitpages.knut.univention.de/talks/dpkg-maint.html) von damals.
-->

{% include abbreviations.md %}
