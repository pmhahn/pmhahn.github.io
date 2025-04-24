---
title: 'Python 3.9: String Literals'
date: '2020-04-14T13:00:24+02:00'
layout: post
categories: python
---

Ein Rätsel:

```console
$ cd test/ucs-test && ucslint
Process Process-1:
Traceback (most recent call last):
...
SyntaxError: (unicode error) 'unicodeescape' codec can't decode bytes in position 32-34: truncated \UXXXXXXXX escape

During handling of the above exception, another exception occurred:

Traceback (most recent call last):
...
AttributeError: 'NoneType' object has no attribute 'rstrip'
```

Keine Idee? Hier der nächste Hinweis:

```bash
python3 -m flake8 tests/51_samba4/56evaluate_windows_gpo
```

mit dem selben Traceback.

Immer noch keine Idee? Den nächsten Hinweis bekommt man mit einem Blick in die [Datei](https://github.com/univention/univention-corporate-server/blob/4.4-4/test/ucs-test/tests/51_samba4/56evaluate_windows_gpo#L508):
```python
windows_set_gpo_registry_value(gpo_name,
 "HKCU\Software\Policies\Microsoft\UCSTestKey",
 "TestUserValueOne",
 "Foo",
 "String")
```

Immer noch nicht?
```
 "HKCU\Software\Policies\Microsoft\UCSTestKey"
                                  ^^12345678
```

`\U........` ist eine Escape-Sequenz für [String literals](https://docs.python.org/3/reference/lexical_analysis.html#strings), um Unicode-Zeichen angeben zu können.
Nach dem `\U` müssen exakt 8 Hex-Digits kommen – `CSTestKe` ist das nicht.
(Der kleine Bruder wäre übrigend `\u....`, was nur 4 Hex-Zeichen erwartet.)

Doch warum tritt das nur mit Python 3 auf?

Nun, mit [Python 3 ist unicode Standard]({% post_url 2020-03-10-python-3-unicode %}).
Da die beiden Escape-Sequenzen aber eben nur für Unicode-String (und nicht für Byte-Strings) aktiv sind, tritt das Problem erst mit Python 3 auf.

# Merke

Zeichenketten, die den Backslash enthalten, sollten nach Möglichkeit als sog. **Raw-String** `r"…\…"` notiert werden.
Das der Backlash sehr häufig in [regulären Ausdrücken](https://docs.python.org/3/library/re.html) genutzt wird, ist es dort besonders wichtig.

PS: Ich habe ein Skript, was ich noch in `ucslint` integrieren will. [flake8](https://flake8.pycqa.org/en/latest/index.html) liefert dafür eigentlich auch schon [Invalid escape sequence ‚x‘ (W605)](https://www.flake8rules.com/rules/W605.html), aber wenn es bei einer ungültigen Sequenz abstürzt, hilft einem das auch nicht viel weiter. Eine [Korrektur von ucs-test](https://github.com/univention/univention-corporate-server/commit/00e37d83c7750e4eefec3d63beac1dbc2abc545f) habe ich bereits.

PPS: Und als Vorfreude auf UCS-5 basierend auf Debian 10 Buster mit Python 3.7 haben wir dann auch irgendwann mal _Python 3.6 f-Strings_.
