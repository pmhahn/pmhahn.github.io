---
title: 'The Unicode desaster'
date: '2013-04-17T18:30:42+02:00'
layout: post
categories: python
---

Wenn man das Zeichen-Encoding richtig machen will, empfiehlt es sich, intern mit Unicode zu arbeiten und bei jedem Zugriff auf das Dateisystem die Daten zwischen dem internen Unicode-Format und der externen Kodierung zu konvertieren.
Leider ist Python-2 aus [Kompatibilitätsgründen](http://fedoraproject.org/wiki/Features/PythonEncodingUsesSystemLocale) dazu gezwungen, sein altes kaputtes Verhalten beizubehalten.
Besser wird es erst mit Python 3, wo dann alle Zeichenketten standardmäßig Unicode verwenden und man explizit sagen muß, wenn man ein Byte-Array haben möchten.
Kaputt deswegen, weil:

```console
$ python -c 'print unichr(0xfc)'
ü
$ python -c 'print unichr(0xfc)' | cat
UnicodeEncodeError: 'ascii' codec can't encode character u'\xfc' in position 0: ordinal not in range(128)
```

Pyhton erkennt hier, ob stdin, stdout, stderr TTYs sind und verwendet nur dann die `LC_CTYPE`-Locale-Einstellung;
ansonsten bleibt es bei 7-Bit-ASCII.
Wenn man dann probiert einen Unicode-String auszugeben, weiß Python den nicht zu konvertieren und gibt den TraceBack aus.

Eine generische Lösung für stdin/-out/-err ist bei [stackoverflow](http://stackoverflow.com/questions/492483/setting-the-correct-encoding-when-piping-stdout-in-python) beschrieben:

```python
from locale import getdefaultlocale
import codecs
import sys
LANG, ENCODING = getdefaultlocale()
sys.stdin = codecs.getreader(ENCODING)(sys.stdin)
sys.stdout = codecs.getwriter(ENCODING)(sys.stdout)
sys.stderr = codecs.getwriter(ENCODING)(sys.stderr)
```

Wenn man dann noch statt `open(filename, mode)` besser nachfolgenden Code verwendet, dann ist man schon ein ganzes Stück weiter, ein portables Python-Programm schreiben zu können, das auch mit unterschiedlichen Encodings umgehen kann:
```python
import codecs
codecs.open(filename, mode, encoding=ENCODING)
```

{% include abbreviations.md %}
