---
layout: post
title: "AVM FRITZ!Smart Energy 200 CSV Desaster"
date: 2025-04-30 14:24:00  +0200
categories: linux
excerpt_separator: <!--more-->
---

Ich besitze privat mehrere (~25) Geräte meines Arbeitgebers [AVM GmbH](https://avm.de/), darunter u.a. ein schaltbare Steckdose [FRITZ!Smart Energy 200](https://fritz.com/produkte/smart-home/fritzsmart-energy-200/), früher bekannt als _FRITZ!DECT 200_.
Diese erfasst auch den Energieverbrauch der angeschlossenen Geräte.
Diese werden aggregiert für 24 Stunden, 1 Woche, 1 Monat oder 1-2 Jahr als CSV-Datensatz zur Verfügung gestellt.
Diesen kann man sich entweder bei Bedarf über die Web-Oberfläche der FRITZ!Box herunterladen oder sich regelmäßig als Push-Service-Mail zuschicken lassen.

<!--more-->

# Prozedur

Zunächst ein Blick auf die Prozedur, wie man an die Daten kommt.

## Kritik 1: Push-E-Mail

Der Push-Service hat (zumindest bei mir) jahrelang nicht funktioniert.
Ursache war, dass ich irgendwann meinen Provider gewechselt habe.
Als Absenderadresse war aber weiterhin die Anmeldekennung meines alten Providers als Absender-E-Mail-Adresse eingetragen.
Die hat dann irgendwann nicht mehr funktioniert und von da an landeten alle Mail im Nirgendwo.

Bitte in der Benutzeroberfläche irgendwo anzeigen, wenn es Probleme mit der Push-E-Mail gibt.

## Kritik 2: FRITZ!Box Web-Oberfläche

Die Navigation durch die Web-Oberfläche ist auch alles andere als intuitiv:
1. Die Konfiguration des globalen Push-Services ist unter _System_ → _Push Service_
2. In der _Übersicht_ wird die 200 zwar als _Smart Home_ gerät angezeigt, aber nicht mit einem direkten Link zu deren Einstellungen
3. Diese findet man erst unter _Smart Home_ → _Geräte und Gruppen_ → _Gerätename_ → _Einstellungen_ → _Allgemein_ bzw. _Energieanzeige_ → _Gesamtenergie (kWh)_

Bitte verkürzt den Weg, um an die Information zu kommen.

## Kritik 3: API

Für die automatische Weiterverarbeitung ist E-Mail suboptimal:
1. Man muss diese irgendwie per IMAP oder POP3 abholen
2. Man muss die E-Mail parsen und nach dem CSV-Anhang durchsuchen
3. Man muss dann die Daten irgendwo ablegen.

Den Download von der Web-Oberfläche kann man auch nicht von extern aufrufen.

Bitte schafft eine API, über die man sich die Informationen ohne viel Aufwand herunterladen kann.
Für die Authentifizierung sollte einen standardisierten Mechanismus wie Benutzername-Passwort, HTTP-Header-Token, oder ähnliches verwendet werden.

## Kritik 4: Dateinamen

Die Dateinamen der CSV-Dateien folgen 2 Schemata, je nach dem ob man sich die Datei per Push-Service zuschicken lässt oder sie von der Web-Oberfläche herunterlädt:
1. `YYYYmmdd-HHMMSS-idXXXXX_ZEITRAUM.csv` (Push-Service-E-Mail)
2. `$NAME_dd.mm.YYYY_HH-MM_ZEITRUM.csv` (Download)

- `NAME` ist der eingestellte Gerätename, die `ID` eine willkürliche(?) Nummer.
  Warum ist der in der Push-Service-E-Mail nicht enthalten?
  Den Namen des Geräts muss man sich also anderweitig aus der E-Mail parsen, sofern man mehrere Geräte hat.
  Die `ID` taucht an keiner anderen Stelle nochmals auf.
- Warum enthält die eine Variante _Sekunden_, die andere nicht?
- `ZEITRAUM` ist `24h` oder `week` oder `month` oder `2years`.  
  **Ausnahme**: Beim Push-Service ist es `1month`, also mit vorangestellter `1`.

Bitte vereinheitlicht die Dateinamen für den Push-Service und den Download.

------

# Kopfzeilen

Schauen wir uns nun den Inhalt der CSV-Dateien genauer an:
Diese haben grob folgenden Aufbau:
```
sep=;
Kopfzeile
Datenzeilen…
```

## Kritik 5: CSV Format

[CSV-Dateien](https://de.wikipedia.org/wiki/CSV_(Dateiformat)) sind zwar einfach zu erzeugen, aber deren Weiterverarbeitung ist alles andere als trivial, weil es viele Unterformate gibt:
- unterschiedliche **Zeichenkodierungen**, e.g. *ASCII*, *UTF-8*, *UTF-16*, *ISO-8859-1*, ̇…
- unterschiedliche Zeichen für den **Zeilenumbruch**, e.g. _LF_ (`\n`), _CR_ (`\r`), _CR_+_LF_ (`\r\n`), ̇…
- unterschiedliche **Trennzeichen** für die Spalten: _Komma_ (`,`), _Semikolon_ (`;`), _Tabular_ (`\t`), Leerzeichen (` `), …
- **Zeichenketten** in _einfache_ (`'`) bzw. _doppelte Anführungszeichen_ (`"`) einschließen oder nicht
- unterschiedliche **Dezimaltrennzeichen** für Zahlen, e.g. _Punkt_ (`.`) oder _Komma_ (`,`)
- **Escape-Mechanismus** für Zeichen, sie ansonsten als Trennzeichen interpretiert würden
- initiale **Leerzeichen** nach Trennzeichen sind relevant oder werden ignoriert
- Datei enthält eine **Kopfzeile** oder beginnt direkt mit der ersten **Daten-Zeile**

Bitte stellt die Daten in einem strukturierten Format zur Verfügung, das einfach zu verarbeiten ist und nach Möglichkeit eine eindeutige Semantik hat.

## Kritik 5: CSV Trennzeichen

Die Datei beginnt mit einem `sep=;`.
Es handelt sich um eine Excel-Erweiterung, die von vielen anderen CSV-Parsern nicht verstanden wird.
Sie ist nicht Bestandteil von [RFC 4180: Common Format and MIME Type for Comma-Separated Values (CSV) Files](https://www.rfc-editor.org/rfc/rfc4180).
In [W3C: Model for Tabular Data and Metadata on the](https://www.w3.org/TR/tabular-data-model/#h-sotd) wird lediglich erwähnt, das manche Programm so _Metadaten_ ablegen.
Davon wird davon abgeraten, denn es führt gerne zu Problemen:
- Der [Python-Parser](https://docs.python.org/3/library/csv.html#csv.DictReader) erkennt z.B. nur Kopfzeilen, wenn sie in der ersten Zeile sind.
  Die zusätzliche Zeile mit dem `sep=;` zerstört diesen Mechanismus.

Bitte diese Zeile entfernen.

## Kritik 6: Kopfzeile uneinheitlich

Als nächstes folgt die Kopfzeile.
Normalerweise dient dieser der Benennung der Spalten.
Hier ein paar Beispiele[^1]:

[^1]: Für die bessere Lesbarkeit habe ich Leerzeichen eingefügt, um die Spalten besser kenntlich zu machen. 

```
1            |2             |3      |4                |5      |6           |7      |8|9       |10        |11|12     |13
Datum/Uhrzeit;Verbrauchswert;Einheit;Verbrauch in Euro;Einheit;CO2-Ausstoss;Einheit; ;Ansicht:;Datum     ;  ;1 Monat;dd.mm.YYYY HH:MM Uhr
Datum/   Zeit;Energie       ;Einheit;Energie   in Euro;Einheit;CO2-Ausstoss;Einheit; ;Ansicht:;1 Monat   ;  ;Datum  ;dd.mm.YYYY HH-MM Uhr
Datum/   Zeit;Energie       ;Einheit;Energie   in Euro;Einheit;CO2-Ausstoss;Einheit; ;Ansicht:;1 Woche   ;  ;Datum  ;dd.mm.YYYY HH-MM Uhr
Datum/   Zeit;Energie       ;Einheit;Energie   in Euro;Einheit;CO2-Ausstoss;Einheit; ;Ansicht:;24 Stunden;  ;Datum  ;dd.mm.YYYY HH-MM Uhr
Datum/   Zeit;Energie       ;Einheit;Energie   in Euro;Einheit;CO2-Ausstoss;Einheit; ;Ansicht:;2 Jahre   ;  ;Datum  ;dd.mm.YYYY HH-MM Uhr
```
1. Spalte 1 heißt **uneinheitlich** mal `…/Uhrzeit`, manchmal nur `…/Zeit`.
2. Spalte 2 heißt **uneinheitlich** mal `Energie`, manchmal aber `Verbrauchswert`.
3. Spalten 3, 5 und 7 haben jeweils die Überschrift `Einheit` und geben die physikalische Einheit der Spalte davor an.
   Die Namen der Spalten sollten besser eindeutig sein, denn manche CSV-Parser erlauben keine doppelten Spaltennamen.
4. Spalte 4 heißt **uneinheitlich** `Verbrauch in Euro`[^2], manchmal aber `Energie in Euro`[^3].
   Warum steht hier überhaupt die Einheit _Euro_ in der Überschrift, obwohl es dafür doch eine eigene Spalte 5 gibt?
5. Spalten 8-13 sind nur in der Kopfzeile zu finden.
   Sie benennen hier deswegen nicht die Spalten, sondern enthalten _Meta-Daten_ über die gesamte Datei.
6. Spalten 8 und 11 sind leer.
   Das verwirrt manche Parser.
   LibreOffice z.B. erlaubt es, solche leeren Spalten zu ignorieren.
7. Spalten 10 und 12 sind manchmal **vertauscht**:
   Manchmal steht der _Zeitraum_ in Spalte 10, manchmal aber auch in Spalte 12.
8. Spalte 13 enthält dern _Zeitpunkt_, zu dem die Datei generiert wurde.
   Die Stunden sind von den Minuten durch unterschiedliche **Trennzeichen** separiert: manchmal mit einem _Doppelpunkt_ (`:`), manchmal mit einem _Minus-Zeichen_ (`-`).

Bitte eine einheitliche und konsistente Kopfzeile erzeugen!

[^2]: Als Physiker störe ich mich am Wort _Verbrauch_, denn Energie wir nach dem [1. Hauptsatz der Thermodynamik](https://de.wikipedia.org/wiki/Thermodynamik) nicht _verbraucht_, sondern (u.a. in Wärmeenergie) _umgewandelt_.

[^3]: [Energie](https://de.wikipedia.org/wiki/Energie) ist auch der falsche Begriff.
      Korrekt wäre Energie**kosten**.
      Und die Einheit für _Energie_ wäre _Joule_, nicht _Euro_.

------

# Datensätze

Je nach Zeitraum haben die Datensätze ein unterschiedliches Format für die 1. Spalte mit dem _Datum/[Uhr]zeit_:
Man benötigt also pro Format einen eigenen Parser.

## Kritik 7: Tag / 24h

Die Datei mit den Datensätzen für einen Tag enthält für die letzten 24 Stunden jeweils 4 Datensätze im Abstand von 15 Minuten.
Die erste Spalte sieht wie folgt aus:

```
23:45;…
0:00;…
jetzt;…
```

Ohne Kontextwissen sind diese Zeitstempeln nicht zu interpretieren:
1. Man benötigt den Erstellungszeitpunkt der Daten aus der Kopfzeile oder dem Dateinamen, um das korrekte Datum zu ergänzen.
2. Man muss selber erkennen, zwischen welchen Zeilen der Datumswechel stattgefunden hat.
3. Das `jetzt` erfordert eine weitere Sonderbehandlung.

Es bleibt unklar, ob die Uhrzeit sich auf den _Beginn_ oder das _Ende_ der Erfassungsperiode bezieht.
Von `jetzt` könnte man auf das Ende schließen, aber scheinbar ist es jeweils der **Anfang**.
Von daher ist die Bezeichnung `jetzt` doppelt falsch.

Bitte immer einen kompletten Zeitstempel bestehend aus Datum und Uhrzeit angeben.  
Bitte dokumentieren, ob es sich um den _Beginn_ oder das _Ende_ der Erfassungsperiode handelt.

## Kritik 8: Woche

Die Datei mit den Datensätzen für eine Woche enthält für die letzten 7 Tage jeweils 4 Datensätze im Abstand von 6 Stunden.
Die erste Spalte sieht wie folgt aus:

```
12;…
18;…
Do.;…
6;…
```

Ohne Kontextwissen sind diese Zeitstempeln nicht zu interpretieren:
1. Man benötigt den Erstellungszeitpunkt der Daten aus der Kopfzeile oder dem Dateinamen, um das korrekte Datum zu ergänzen.
2. Statt 0 Uhr wird der _Wochentag_ benannt, der aber ohne Kontextwissen nutzlos bleibt.
   Zudem ist unklar, ob der Wochentag lokalisiert ist, d.h. die Wochentage der Spracheinstellung nach unterschiedlich benannt werden.
3. Die unterschiedlichen Datentypen _Wochentag_ und _Stunde_ machen das Parsen nur komplizierter.
4. Die Zeilen müssen in genau dieser Reihenfolge verarbeitet werden.
   Sie dürfen auf keinen Fall umsortiert werden, weil die Zeile _Stunden_ ansonsten nicht mehr eindeutig einem Wochentag zugeordnet werden können.

Es bleibt unklar, ob die Uhrzeit sich auf den _Beginn_ oder das _Ende_ der Erfassungsperiode bezieht.
Vermutlich das **Ende**.

Bitte immer einen kompletten Zeitstempel bestehend aus Datum und Uhrzeit angeben.  
Bitte dokumentieren, ob es sich um den _Beginn_ oder das _Ende_ der Erfassungsperiode handelt.

## Kritik 9: Monat

Die Datei mit den Datensätzen für einen Monat enthält für die letzten 31 Tage jeweils einen Datensatz pro Tag.
Die erste Spalte sieht wie folgt aus:

```
31.12.;…
1.1.;…
```

Ohne Kontextwissen sind diese Datumsangaben nicht zu interpretieren:
1. Man benötigt den Erstellungszeitpunkt der Daten aus der Kopfzeile, um das korrekte Jahr zu ergänzen.
2. Man muss selber erkennen, zwischen welchen Zeilen der Jahreswechsel stattgefunden hat.
3. Es werden immer 31 Tage gelistet, auch wenn der Monat nur 30/29/28 Tage hat.
   Aggregiert man mehrere Dateien, so muss man auf die Überlappung der Tage achten und diese ggf. extra behandeln.

Die Angabe bezieht sich vermutlich auf einen kompletten Tag, also von 00:00 Uhr bis 00:00 Uhr des Folgetags.
Bei der Umwandung in einen Zeitstempel muß man also `00:00:00` bzw. `23:59:59` als Uhrzeit ergänzen, je nach dem ob man mit dem _Anfang_ oder _Ende_ rechnet.

Bitte immer einen komplettes Datum inklusive Jahreszahl angeben.

## Kritik 10: Jahr

Die Datei mit den Datensätzen für die letzten 1-2 Jahre enthält für jeden Monat jeweils einen Datensatz.
Die erste Spalte sieht wie folgt aus:

```
Mai 2023;…
Juni 2023;…
```

1. Es bleibt unklar, wie die Monate lokalisiert werden, d.h. wie sie je nach Spracheinstellung benannt werden.

Die Angabe bezieht sich vermutlich auf einen kompletten Monat, also von 00:00 Uhr des 1. Tags inklusive bis 00:00 Uhr des 1. Tags des Folgemonats exklusive.
Im Detail bleibt aber auch hier unklar, ob intern nicht auch einfach immer mit 31 Tagen pro Monat gerechnet wird.
Bei der Umwandung in einen Zeitstempel muß auch hier darauf geachtet werden, ob mit dem ersten oder letzten Tag des Monats gearbeitet wird und welche Uhrzeit verwendet wird.

Bitte Datums-Angaben nicht lokalisieren.

## Kritik 11: Daten nicht konstant

Exportiert man die Daten mehrfach hintereinander, stellt man fest, das diese für identische Zeiträume nicht identisch sind:
Sie unterscheiden sich zwar nur um wenige Watt, aber dennoch ist das unschön.
Vermutlich ist das der [Round-Robin-Datenbank](https://oss.oetiker.ch/rrdtool/) geschuldet, die intern _Sampling_ verwendet, um (fehlende) Werte zu interpolieren.

Bitte eine Datenbank verwenden, die reproduzierbar die selben Daten liefert.

------

# Fazit

Innerhalb des FRITZ-Ökosystems funktionieren die Produkte ja wunderbar miteinander, aber der Export der Daten für die Weiterverarbeitung in einem anderen System ist eine Katastrophe.
Insbesondere CSV als Format sehe ich als sehr problematisch, da die Weiterverarbeitung alles andere als einfach ist.
Eine fehlenden API für den Abruf der aktuellen Daten per Skript macht es noch komplizierter.

Mein in Python geschriebener Parser inklusive einigen Testfällen und Validierung der Zeichenketten bringt es aktuell auf 324 Zeilen.
Nicht gerade wenig für ein Programm, dass nur CSV-Dateien parsen und sie in ein einheitliches Format bingen soll.

------

{% include abbreviations.md %}
