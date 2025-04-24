---
title: 'Konfigurationsdateien bei Paketupgrade handhaben'
date: '2011-08-30T12:36:57+02:00'
layout: post
categories: debian
---

`dpkg` behandelt alle Dateien unterhalb von `/etc/` gesondert, da es sich dabei um sog. **conffiles** handelt: "Änderungen daran durch den Benutzer müssen laut Debian-Policy selbst bei einem Paket-Upgarde erhalten bleiben." Diese werden auch nicht bei einem normalen **remove** entfernt, sondert erst bei einem **purge**. Dazu speichert `dpkg` für jede Konfigurationsdatei in `/var/lib/dpkg/status` die md5-Summe der Originaldatei, um geänderte Dateien zu erkennen. (Diese lassen sich durch `dpkg-query -W -f '${Conffiles}\n' "$pkg_name"` auslesen).

Bei einem Upgrade eines Pakets bleiben alte conffiles bestehen, selbst wenn diese im neuen Paket nicht mehr mitgeliefert werden. Deswegen ist hier Handarbeit im `preinst`-Skript des Pakets notwendig, wie es ausführlich unter [http://wiki.debian.org/DpkgConffileHandling](http://wiki.debian.org/DpkgConffileHandling "DpkgConffileHandling") beschrieben wird. Dort wird dem dem Fall, das eine Datei gelöscht werden soll, auch der Fall behandelt, das eine Datei umbenannt wird.

Für Templates sollte man daran denken, das neben der eigentlichen Datei `/etc/$path. auch noch weitere Dateien wie die Template-Datei `/etc/univention/templates/files/etc/$path` unterhalb von `/etc/` liegen können, die ebenfalls behandelt werden sollten.

Bisher gibt es dafür weder einen Helper, noch eine Vorlage, aber ggf. motiviert dieses Posting ja den Einen oder Anderen dazu 😉

{% include abbreviations.md %}
