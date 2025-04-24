---
title: 'Python 3.3: unicode'
date: '2020-03-10T23:39:42+01:00'
layout: post
categories: python
---

In Python 2 sind Zeichenketten intern eine Ansammlung von Bytes, in Python 3 dagegen **Unicode-Zeichen**.
Letztere gibt es auch schon in Python 2, nur musste man sie dort explizit als solche Deklarieren.
Nun gilt es umgekehrt und man muss Bytes explizit in Python 3 deklarieren.

|      | Python 2 | Python 3 |
| ---- | -------- | -------- |
| `b"` | bytes    | bytes    |
| `"`  | bytes    | unicode  |
| `u"` | unicode  | unicode  |

## UTF-8

Über ein `from __future__ import unicode_literals` kann man auch in Python 2 schon das Verhalten ändern und bekommt dann auch über `'…'` bereis eine `unicode`-Zeichenkette.

Der Teufel steckt aber im detail:
Die Unicode-Zeichenkette `s = u"äöü"` besteht aus 3 Unicode-Zeichen, d.h. `len(s) == 3`.
Intern reserviert Python für jedes Unicode-Zeichen entweder 2 oder 4 Byte, was beim beim Übersetzten des Python-Interpreters auswählen kann.
Ersteres spart Platz, hat aber den Nachteil, dass eben nur die ersten 65536 Unicode-Codepoints verarbeitet werden können, der derzeit aktuelle [Unicode Standard 12](https://home.unicode.org/) enthält aber jetzt schon ~130k Zeichen, weshalb aktuelle Python-Versionen intern alle mit 32 Bit pro Zeichen arbeiten.
Intern belegt also unsere Zeichenkette 12 Byte zzgl. Längeninformation, was als Hex-Dump dann so aussieht:

```
00 00 00 e4  00 00 00 f6  00 00 00 fc
```

Auffällig sind die vielen `00`, was Erstens nach Platzverschwendung aussieht, und Zweitens dazu führt, dass die klassischen C-Routinen für die Verarbeitung von Zeichenketten damit nicht klar kommen, weil für sie ein `"\0"` das Ende der Zeichenkette signalisiert.

Kluge Köpfe haben sich deswegen [UTF-8](https://de.wikipedia.org/wiki/UTF-8) erdacht, was für **UCS Transformation Format 8** steht – **UCS** steht hier aber für [Universal Coded Character Set](https://de.wikipedia.org/wiki/Universal_Coded_Character_Set), was wir hier gleichsetzten können mit Unicode.
UTF-8 verwendet eine Kodierung **variabler** Länge, d.h. manche Unicode Zeichen brauchen nur 1 Byte, andere bis zu 4 Byte – theoretisch sogar bis zu 8 Byte.
Die ersten 128 Zeichen von Unicode, die dem 7-bittigen [ASCII](https://de.wikipedia.org/wiki/American_Standard_Code_for_Information_Interchange)-Alphabet entsprechen, benötigen 1 Byte, d.h. englischsprachige Texte ohne Umlaute und sonstige Sonderzeichen benötigen keinen zusätzlichen Platz.
Die Unicode-Zeichen 128 bis 1920 dagegen benötigen 2 Byte – in dem Bereich liegen z.B. die deutschen Umlaute, d.h. nur für diese werden dann 2 Byte benötigt, die restlichen „normalen“ Buchstaben weiterhin 1 Byte.
Die Unicode-Zeichen 1920 bis 65.536 benötigen 3 Byte, die Zeichen bis 2.097.152 dann 4 Byte.
Vorteil von UTF-8 ist, dass nirgendwo `\0` vorkommt und man deshalb solche UTF-8 kodierten Texte weiterhin mit klassischen UNIX-Tools verarbeiten kann.

Zur Umwandlung von der **internen** Unicode-Speicherung in eine UTF-8-Sequenz verwendet man `encode()`:
```pytho
u'ä ö ü'.encode('utf-8') == b'\xc3\xa4 \xc3\xb6 \xc3\xbc'
```

Den Rückweg übernimmt `decode()`:
```python
b'\xc3\xa4 \xc3\xb6 \xc3\xbc'.decode('utf-8') == u'ä ö ü'
```

Aber:
man muss aufpassen, denn man darf eine UTF-8 Sequenz nicht an beliebiger Stelle aufspalten:
Denn nimmt man nur das erste Zeichen unseres 8 Byte langen Byte-Stroms `c3`, handelt es sich dabei nicht um eine gültige UTF-8-Sequenz, denn sie ist unvollständig:
diese muss in diesem Fall 2 Byte lang sein.
D.h. man sollte UTF-8 kodierte Daten nicht an beliebigen Stellen auftrennen, sondern möglichst an einer Stelle davor oder dahinter, so dass die UTF-8-Sequenz vollständig ist.
(Deswegen gibt es z.B. auch bei `cut` den Unterschied zwischen `--bytes` und `--characters`:
Ersteres zerhackt ggf. eine UTF-8-Sequenz irgendwo zwischendrin und führt zu einem unvollständigen letzten Zeichen).

Aufpassen muss man deshalb u.a. dann, wenn man UTF-8-Sequenzen über das Netzwerk überträgt und dort z.B. Pakete einer festen Länge schnürt:
Ein Empfänger, der den Datenstrom dann zeichenweise verarbeitet, muss dann eben damit umgehen können, dass er die UTF-8-Sequenz eines Zeichens noch nicht komplett empfangen hat und dieses erst dann verarbeiten kann, wenn er das nächste Datenpaket mit dem Rest der Sequenz erhalten hat.

Dazu muss man natürlich wissen, dass es sich bei den Byte-Daten um eine **Zeichenkette** handelt und dass dieses die **UTF-8-Kodierung** verwendet!
Lese ich z.B. die Binärdaten eines JPEG-Fotos wäre schon die erste Annahme falsch;
lese ich einen Text von einem Windows-System, könnte dieser auch die dort öfters gebräuchliche **UTF-16**-Kodierung verwenden.
Ebenso kann mir das auch unter Linux passieren, wenn ich noch eine alte Textdatei mit **Latin-1** (ISO-8859-1)-Kodierung habe.

Ob es sich um eine Textdatei handelt und welche Kodierung diese verwendet wird oft nicht explizit gespeichert, d.h. diese Informationen ergeben sich (hoffentlich) implizit.
Heutzutage kann man **UTF-8** annehmen, aber es ist eben nicht garantiert.
(Strenggenommen ergibt es sich für einen Prozess aus seiner `locale`-Einstellung für `LC_CTYPE`, was heutzutage sowas ist wie `de_DE.UTF-8`).
D.h. wenn man sauber programmiert muss man immer damit rechnen, dass man eben keine gültige und vollständige UTF-8-Sequenz als Eingabe bekommt und das eben dann sauber über eine Fehlerbehandlung abfangen sollte.

## byte vs unicode

In Python 2 ist folgendes erlaubt:
```python
b"" + u""
```
Python 2 wandelt implizit das `bytes()`-Array in ein `unicode()`-Array und erlaubt das aneinanderhängen. Über `/etc/python2.7/sitecustomize.py` haben wir in UCS es so konfiguriert, dass Python hier implizit UTF-8 annimmt, weshalb das auch für Umlaute funktioniert.

Python 3 dagegen steigt mit einem Fehler aus:

```
TypeError: can't concat str to bytes
```

D.h. man muss dort sehr viel vorsichtiger sein und tunlichst darauf achten, ob man nun auf `bytes()` oder `unicode()`-Zeichenketten arbeitet.

Die goldene Regel lautet:

> Wenn du Texte von extern liest (Datei, sys.stdin, sys.argv, socket.recv(), subprocess.PIPE, …), dekodiere sie dort per `bytes.deocde("utf-8")`, so dass du dich dann anschließend nicht mehr um dieses Detail kümmern musst und komfortabel auf `unicode()`-Zeichenketten arbeiten kannst.
>
> Wenn du dann Texte nach extern schreiben willst (Datei, sys.stdout, socket.send(), …), kodiere sie dort kurz vorher per `unicode.encode("utf-8")` und gebe das dann aus.

Und ab hier unterschieden sich Python 2 und Python 3 dann in den Details, was das Schreiben von kompatiblen Code erschwert:

- `sys.argv` ist in Python 2 eine `List[bytes]`, unter Python 3 aber eine `List[unicode]`. D.h. will man auch in Python 2 mit `unicode()` arbeiten, muss man die Argumente explizit dekodieren, Python 3 macht es standardmäßig.
- `open(filename, "r")` liefert einem unter Python 2 `bytes()`, unter Python 3 dagegen `unicode()`. Will man unter Python 3 Binärdaten verarbeiten, sollte man explizit `open(filename, "rb")` verwenden. Will man immer mit Unicode arbeiten, kann man in beiden besser `io.open(filename, "r")` verwenden. Gleiches gilt natürlich für das Schreiben mit `w`.
- Python 3 setzt `sys.stdin, sys.stdout, sys.stderr` dankenswerterweise schon mit passenden Konvertern aus, dass man sich nicht explizit selber um die Kodierung kümmern muss: Ein `print(u"ä")` ruft implizit `encode('utf-8')` auf. Auch mit Python 2 funktioniert das wegen unserer geänderten Einstellung unter UCS, **außer** man leitet die Ausgabe in eine Pipe um, denn dann verenden StdIn, StdOut, StdErr plötzlich `ASCII` und man muss einiges an [Aufwand]({% post_url 2013-04-17-the-unicode-desaster %}) treiben, um auch dann Unicode automatisch konvertiert zu bekommen.
- `"ä".isalpha()` ist unter Python 3 `True`, weil hier Python 3 die Unicode-Datenbank konsultiert. Unter Python 2 dagegen `False`, weil hier (standardmäßig) die Byte-Sequenz angeguckt wird und `'\xc3'.isalpha()` eben `False` ist; mit `from __future__ import unicode_literals` bzw. `u"ä".isalpha()` bekommt man auch mit Python 2 ein `True`.
- Auch unterschieden sich die Ausgaben zwischen Python 2 und 3, wenn man `bytes()` oder `unicode()` ausgibt:
    |              | Python 2 | Python 3 |
    | ------------ | -------- | -------- |
    | `print(b"")` |          | b""      |
    | `print(u"")` |          |          |

    Problematisch ist das z.B. bei UDM, weil dieses einen `eval(repr(data))` Zyklus für die Kommunikation zwischen UDM-CLI und UDM-Server über den internen UNIX-Socket verwendet: Durch die unterschiedliche Darstellung hat man dann plötzlich doppelt encodierte Daten und bekommt die seltsamsten `TypeError`-Meldungen.
- In Python 2 gibt es `basestring` als gemeinsame
    Oberklasse von `str=bytes` und `unicode`. In Python 3 gibt es die nicht mehr und man sollte sich davor hüten, das einfach durch `six.string_types` zu ersetzten, denn dann läuft man wieder Gefahr, dass man `bytes()` mit `unicode()` vermischt und einen `TypeError` bekommt. Besser ist es explizit auf den richtigen Type `six.text_type` bzw. `six.binary_type` zu testen.
- Der Vergleich von `bytes()` und `unicode()` unterscheidet sich je nach Python-Version: ```

    ```console
    $ python2 -c 'print(b"" == u"")'
    True
    $ python3 -c 'print(b"" == u"")'
    False
    $ python3 -bb -c 'print(b"" == u"")'
    Traceback (most recent call last):
      File "", line 1, in 
    BytesWarning: Comparison between bytes and string
    ```

    Über die Option `-b` bzw. `-bb` kann man den Python-Interpreter dazu veranlassen, für solche Vergleiche eine Warnung oder Exception auszulösen.
- Das `from __future__ import unicode_literals` ist umstritten und es gibt durchaus die Empfehlung, jeweils explizit ein `u""` oder `b""` zu verwenden.

{% include abbreviations.md %}
