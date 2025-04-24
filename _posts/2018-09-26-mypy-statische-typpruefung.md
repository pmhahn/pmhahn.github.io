---
title: 'mypy: statische Typprüfung'
date: '2018-09-26T16:22:38+02:00'
layout: post
categories: python
---

[mypy](http://mypy-lang.org/) prüft statisch (d.h. zur Entwicklungszeit und nicht während der Laufzeit) Python-Code auf korrekte Datentypen.
Eigentlich ist Python ja untypisiert, was schön ist, weil man nicht Variablken vorab deklarieren muss und damit jede Menge Tipparbeit spart.
Andererseits führt das eben dann ggf. erst zur Laufzeit dazu, dass einem der Code um die Ohren fliegt, weil Typen eben nicht kompatibel sind.
<!-- Teile von UCS enthalten inzwischen bereits die notwendigen Typ-Auszeichungen. Sönke hat zudem dazu mal im [8. Hackathon](https://mail.univention.de/appsuite/#!&app=io.ox/files&folder=1401&id=1401/2637) auch ein Programm geschrieben, was einem hilfe, diese zu erstellen. -->

Neuerdings (zumindest in der aktuelle Version 0.630) hinterlässt `mypy` aber im aktuellen Arbeitsverzeichnis ein verstecktes Verzeichnis `.mypy_cache`, was ich ziemlich nervig finde.
Zum Glück kann man über eine zentrals [Konfigurationsdatei](https://mypy.readthedocs.io/en/latest/config_file.html) `~/.mypy.ini` auch sagen, dass man selber lieber ein zentrales Verzeichnis verwenden will:
```ini
[mypy]
python_version = 2.7
cache_dir = /home/phahn/.cache/mypy/
```

Das funktioniert aber leider nicht mehr, sobald `mypy` eine Datei `pyproject.toml` oder `mypy.ini` im lokalen Verzeichnis findet.
Dann finden die globalen Einstellungen keine Anwendung mehr und man hat doch wieder das versteckte Verzeichnis.
Entweder muss man den Eintrag also in jedem Projekt wiederholen, was aber nicht funktioniert, wenn mehrere Personen gemeinsam an dem Projekt arbeiten und deswegen unterschiedliche Verzeichnisse oder Einstellungen wünschen.

Deswegen kann man die Umgebubgsvariable `MYPY_CACHE_DIR` z.B. in `~/.bashrc` setzten, die dann für alle eignenen Projekte gilt:
```bash
export MYPY_CACHE_DIR="$HOME/.cache/mypy_cache"
export RUFF_CACHE_DIR="$HOME/.cache/ruff_cache"
```

{% include abbreviations.md %}
