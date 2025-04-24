---
title: 'Debian Versions Schema'
date: '2020-06-10T18:03:32+02:00'
layout: post
categories: debian
---

Die [Versionsnummer](man:deb-version(7)) von Debian-Paketen baut sich nach folgendem [Schema](https://www.debian.org/doc/debian-policy/ch-controlfields.html#version) auf:

> [_Epoche_`:`]_UpstreamVersion_[`-`_DebianRevision_]

## Versions-Vergleich

`dpkg --compare-version` kann verwendet werden, um Versionen zu vergleichen.
Grob gesprochen funktioniert der [Vergleich](man:deb-version(7)) wie folgt:

- Die Version wird in zusammenhängende Folgen von Ziffern `[0-9]+` und nicht-Ziffern `[~A…Za…z+-.:]+` aufgeteilt.
- Zusammenhängende Ziffernfolgen werden als Zahl verglichen: `2 < 10`
- Ale anderen Zeichenfolgen werden als Zeichenkette verglichen, wobei Buchstaben aber **vor** den übrigen Sonderzeichen rangieren: `"2" > "10"`
- Die _Tilde_ hat eine Sonderrolle: ihr Wert ist kleiner als der der leeren Zeichenkette, ist also eher **negativ**. Deshalb sortiert sich z.B. `1~something` vor `1` ein: `"~something" < ""`
- Es gibt weitere Einschränkungen, welche Zeichen in welchen Komponenten der Versionsnummer zulässig sind:
    - Die Epoche muss rein numerisch sein,
    - die Upstrem-Version muss mit einer Ziffer beginnen,
    - die Debian-Revision kann kein Minuszeichen enthalten,
    - …

## Upstrean Version

Dafür ist zunächst noch folgender Unterschied wichtig:

### Native packages

Debian **Native** Pakete stammen von Debian selbst, d.h. Debian ist dafür auch Upstream.
Beispiele dafür sind z.B. `dpkg`, `apt` oder alle Pakete des `Debian Installer`s.
Die Annahme hier ist, dass **nur** Debian diese Pakete anfasst, genauer die Maintainer dieser Pakete.
Jede Anpassung führt deshalb zu einer neuen (Upstream)-Version.
Solche Pakete erkennt man daran, dass **kein** Minus-Zeichen in der Versionsnuummer vorkommt.

Das Quellpaket besteht nur aus einem Tape-Archive `${src}_${ver}.tar.*z*` und der zugehörigen [Debian Source Control](man:dpkg-source(1)) Datei `${src}_${ver}.dsc`.

### Upstream packages

Pakete aus externen Quellen tragen **mindestens** ein Minus-Zeichen in der Versionsnummer;
das **letzte** trennt die _Debian Revision_ ab;
alles davor (bis auf die _Epoche_) bezeichnet die Upstream-Version.
Die Annahme hier ist, dass es durchaus für ein-und-die-selbe Upstream-Version mehrere Debian-Revisionen geben kann, bei denen z.B. **nur** die Paketierung durch die Debian-Betreuer verbessert werden.

Die Original-Dateien werden als meist ein Tape-Archive `${src}_${ver}.orig.tar.*z*` nach Möglichkeit 1:1 vom Original-Autor übernommen.
Zusätzlich besteht mit dem Format `3.0 (quilt)` die Möglichkeit, auch **mehrer** Tape-Archive zu verwenden, falls die Upstream-Quelle aus mehreren Archive besteht:
Diese müssen dann dem Schema `${src}_${ver}.orig-${component}.tar.*z*` entsprechen.
Genutzt wird das z.B. von Firefox, wo die Übersetzungen von Upstream als eigenständige Dateien veröffentlicht werden.
Da die Original-Quellen im Verhältnis zu der Debian-Paketierung meist sehr viel größer sind, werden erstere nicht jedesmal neu mit hochgeladen.

## Debian Revision

Die Debian-spezifischen Anpassungen für die Paketrierung wurden früher (Format `1.0`) als `${src}_${ver}.debian.diff.*z*` gepflegt.
Das Format gilt inzwischen als veraltet, weil diese Art von (Mega-)Patch nicht die bevorzugte Variante für die Modifikation ist und es nur noch schwer nachvollziehbar ist, aus welchen einzelnen Patches and Änderungen sich dieser eigentlich zusammensetzt.

Deswegen wir hier meist `3.0 (quilt)` in der Datei `debian/source/format` angegeben.
Der komplette Inhalt des Verzeichnisses `debian/` wird dort einfach als `${src}_${ver}.debian.tar.*z*` veröffentlicht.
Für jede neue Debian-Revision wird die _Debian-Revision_ von den Betreuern um Eins erhöht und bei einer neuen Upstream-Version auf `1` zurückgesetzt.

Für Änderungen an der Upstrean-Version – d.h. für alles **außerhalb** des `debian/`-Verzeichnisses – muss [Quilt](https://wiki.debian.org/UsingQuilt) verwendet werden:
Der Stapel der anzuwendenden Patches wird im Verzeichnis `debian/patches/` abgelegt und deren Reihenfolge über die darin vorhandene Datei `series` festgelegt.
Die Patches werden von [dpkg-source -x](man:dpkg-source(1)) **nach** dem Auspacken der Original-Tape-Archive **automatisch** angewandt.
Beim späteren Paketbau mit [dpkg-buildpackage](man:dpkg-buildpackage(1)) müssen diese aber für den Bau des Quellpakets wieder zurückgenommen werden, **außer** es sollen nur dir  Binärpakete gebaut werden.
Deshalb ist es wichtig, das die Patches nicht _fuzzy_ sind!

Auch hier kommt abschließend noch die `.dsc`-Datei hinzu, die die übrigen Dateien referenziert.

## Epoche

Von Zeit zu Zeit kommt es vor, dass die Upstream-Versionsnummer nicht streng monoton steigend ist, was dann den vergleich anhand der Versionsnummern durcheinanderbringt.
Beispiele für einen solchen Wechsel sind

- Windows 3.1 → 95 → 98 → XP → Vista → 10
- 1alpha → 1beta → 1rc → 1

Die Epoche dient dafür, bei solchen **gravierenden** Änderungen noch eine Möglichkeit zu haben, das die Versionsnummer monoton fortgesetzt werden kann.
Die Verwendung sollte eine **absolute Ausnahme** bleiben, denn sie hat mehrere Nachteile:

- Man wir sie nie wieder los
- Die Epoche ist **nicht** Teil des Dateinames, wohl aber der Meta-Daten. D.h. das Vergleichen von Paketen anhand der Versionsnummer aus dem Dateinamen ist **nicht korrekt** und hat in der Vergangenheit schon mehrfach zu Fehlentscheidungen geführt!

Das Erhöhen der Epoche muss deshalb vorher auf der [debian-devel](https://lists.debian.org/debian-devel/)-Mailingliste diskutiert werden und wird nur als letzter Ausweg akzeptiert.
Deshalb gibt es so kuriose Versionsnummern in Debian wie `1.4+really1.3.35-1`, weil sich das eben wieder auswächst, sobald dann doch mal die richtige Version `1.4` von Debian paketiert und veröffentlicht wird.
(Ursache für solche Versionen sind, das kurz vor dem Release dann Probleme mit `1.4` beobachtet wurden und man deswegen die Rückkehr zur vorherigen `1.3.35` beschlossen hat.)

## Debian Revision Spezial

### Non-Maintainer Upload

Ein Debian-Paket wird von einem Maintainer bzw. einer Gruppe von Betreuern gepflegt.
Nur diese haben das Recht, das Paket anzupassen und eine neue Version hochzuladen.

Von Zeit-zu-zeit kommt es aber vor, das andere Personen ein Paket verändern müssen, z.B. weil die ursprünglichen Maintainer nicht mehr reagieren oder nicht mehr auffindbar sind.
Damit diese Änderungen durch Dritte nicht mit den Änderungen der eigentlichen Maintainer kollidieren gibt es den Mechanismus des [NMU](https://wiki.debian.org/NonMaintainerUpload)s:
Hier wird an _Debian-Revision_, die normalerweise nur aus einer Zahl besteht, eine zweite Zahl separiert durch einen Punkt angehängt: `-1.1` bzw. dann diese zweite Stelle für jedes weitere NMU erhöht.
Erwünscht sind hier nur **minimale Änderungen** am Paket und es ist Aufgabe der Paket-Betreuer, diese Änderungen dann in deren nächste offizielle Version zu übernehmen.
Zu diesem Zeitpunkt wird dann die erste Stelle erhöht und die angehängte zweite Stelle verschwindet wieder: `-2`.

### binNMU

Neben den vorherigen Fällen, bei denen immer auch eine Änderung an den Quellen stattfindet, gibt es von Zeit zu Zeit den Fall, dass ein Paket neu gebaut werden muss, **ohne** dass es eine auslösende Änderung an diesem Quellpaket gibt:

- Wenn sich in einer verwendeten Bibliothek durch eine ABI-Änderung der [soname](https://en.wikipedia.org/wiki/Soname) ändert, muss das Paket gegen die neue Bibliothek neu gebaut werden, um die alte Bibliothek loswerden zu können.
- U.a. die Programmiersprache [go](https://golang.org/) baut **statische** Binaries, d.h. es gibt keine Abhängigkeiten zur Laufzeit auf andere Pakete. Das führt aber z.B. bei Sicherheitsupdates an den verwendeten Komponenten dazu, das **alle** verwendenden Pakete neu gebaut werden müssen.

In Debian kann man deshalb den Neubau eines Pakets **ohne** das Hochladen neuer Quellpaketdateien veranlassen.
Das kann für alle von Debian unterstützen Architekturen passieren, aber auch (und i.d.R.) nur für einzelne.
Intern arbeitet der Mechanismus so, dass neben die Datei `debian/changelog` eine zweite Datei `debian/changelog.$arch` generiert wird, die eine höhere Versionsnummer hat.
Diese setzt sich zusammen aus der **ursprünglichen** Debian-Version mit dem zusätzlichen Anhang `+b1`, `+b2`, usw.
Die so generierte `changelog`-Datei findet man später auch im Paket unter `/usr/share/doc/$pkg/changelog.Debian.$arch.gz`.

Beachte:

- "binNMU"s gibt es nur für architektur-**abhängige** Pakete (`Architecture: any`), nicht für unabhängige Pakete (`Architecture: all`).
- Da "binNMU"s nicht durch eine Quellcode-Änderung ausgelöst sind, bleibt die Versionsnummer der Quellpakete **unverändert**.

## Debian Security Updates und Point Releases

Für das aktuelle Release veröffentlicht Debian regelmäßig [Sicherheitsupdate](https://www.debian.org/security/) in einem getrennten [Software-Depot](http://security.debian.org/debian-security).
Die Pakete werden von dort dann regelmäßig in sog. _Point-Releases_ übernommen und wandern in dem Schritt zurück in das [Haupt-Depot](https://deb.debian.org/).

Damit auch für diese Updates gewährleistet ist, dass die Versionsnummer kontinuierlich ansteigt und nicht mit anderen, teilweise auch schon in anderen Releases veröffentlichten Versionen kollidiert, wird hier `+deb${release}u${update}` an bisherige Paketversion angehängt.
`$release` bezeichnet hier die Nummer des Debian-Releases – `8` für "Jessie", `9` für "Stretch", `10` für "Buster", `11` für "Bullseye", `12` für "Bookworm", `13` für "Trixie" – und `$release` wird kontinuierlich pro Release hochgezählt.

In den früheren [Debian-Releases](https://www.debian.org/releases/) "wheezy"(7), "squeeze"(6), "lenny"(5), "etch"(4), "etchnhalf"(4½) wurde stattdessen ein `+${distribution}` eingefügt, aber man erkennt hier schnell, dass sich diese Zeichenketten schlecht sortieren lassen.

### Debian Backports

Werden neuere Versionen z.B. aus "testing" zurück nach "[old]stable" portiert, so muss gewährleistet sein, dass bei einem späteren Update auf "testing" dann die Version aus "testing" installiert wird.
Deshalb muss die Versionsnummer des Backport **kleiner** sein als die Originalversion aus "testing".
Das wird erreicht durch Anhängen von `~bpo${release}+${update}`.

## Binary Package version

Normalerweise haben alle Binärpakete, die aus einem Quellpaket gebaut werden, die selbe Versionsnummer wie das Quellpaket aus `debian/changelog`.
Das ist aber nicht immer so und es tritt z.B. immer bei [binNMU](#binNMU)s auf.
Daneben gibt es viele weitere Ausnahmen, z.B. die Pakete des Linux-Kernels, von gcc, Firefox oder Thunderbird in Debian:

```console
# grep-dctrl -s Package,Version,Source -F Source ' ' ./Packages
…  
Package: iceweasel-l10n-all
Version: 1:68.8.0esr-1~deb10u1
Source: firefox-esr (68.8.0esr-1~deb10u1)
…
Package: linux-doc
Version: 4.19+105+deb10u4
Source: linux-latest (105+deb10u4)
```

Im Falle von IceWeasel/Firefox liegt es z.B. daran, dass für die Zurückumbenennung von IceWeasel zu Firefox weiterhin noch die [Transitional-Pakete](https://wiki.debian.org/PackageTransition) benötigt werden;
Anstatt diese aber nur weiterhin in einem ansonst leeren Quellpaket weiterhin pflegen zu müssen übernimmt das Paket `thunderbird` nun diese Aufgabe, insbesondere da ansonsten immer zwei Pakete aktualisiert werden müssten.
Beim Linux-Kernel-**Meta**-Paket dagegen handelt es sich sich im ein Debian-Native-Paket, dessen Versionsnummer einfach fortlaufend hochgezählt wird.
Die daraus gebauten Binärpakete beziehen sich daher dann auf eine konkrete Version des Linux-Kernels und sollen von daher auch dessen Version in ihrer Binärversionsnummer tragen.

## UCS

Bis UCS-4.2 wurde für den Neubau der Pakete an die Debian-Version `.${counter}.${timestamp}` angehängt.
Problematisch hier ist, das der "UCS"-Punkt `.` lexikografisch größer ist als fast alle anderen Zeichen und damit das obige Schema von Debian [kaputt](https://forge.univention.org/bugzilla/show_bug.cgi?id=37376) macht:
Alle später von Debian veröffentlichten Pakete sortieren sich dann **vor** den UCS-Neubau ein, weshalb das dann jedesmal eine manuelle Anpassung der Versionsnummer für UCS erzwungen hat.
Da `$counter` i.d.R. sehr viel größer als jeder `$nmu` ist, hat das schon für [NMU](#nmu)s nicht gut funktioniert.
Seit UCS-4.2 verwenden wir deshalb den Suffix `A~${major}.${minor}.${patch}.${timestamp}`, was wegen dem `A~` nur minimal größer ist als die Debian-Version und bisher kleiner als alle sonstigen verwendeten Suffixe wie `+debXuY` bzw. `+ubuntuZ` ist.

Selbst in UCS-5 haben wir mit den Problemen aus unserem alten Schema zu kämpfen:
In Debian gibt es sehr viele Pakete, die seit vielen Jahren unverändert bestehen und für die insbesondere keine neue Upstream-Version veröffentlicht wurden – sie sind eben sehr stabil.
Da diese Pakete immer noch mit unserm alten Versionsschema gebaut wurden, blockieren sie auch immer noch den direkten Import aus Debian und müssen von uns weiterhin mit einer von Hand angepassten Versionsnummer neu gebaut werden:
Mit `build-package-ng --version "$version"` kann man explizit die gesamte Versionsnummer angeben oder mit `build-package-ng --build "$infix"` etwas zwischen der Debian-Version und dem UCS-Anhängsel einfügen.

## Fazit

Die Versionsnummer enthält einiges an Zusatzinformation, die man richtig interpretieren muss.
Man sollte sich davor hüten, einfach die Versionsnummer aus dem Dateinamen zu extrahieren, den dort fehl ggf. die Epoche.
Richtig und besser ist meist das Parsen der `Packages`-Dateien, denn dort findet man alle wichtigen Informationen zu `Package`, `Version` und `Source`, wie es auch von `repo-ng` inzwischen implementiert wird.
Auch sollte man nach Möglichkeit `dpkg --compare-version` oder `apt_pkg.version_compare()` verwenden, denn die Regeln für den Vergleich sind trickreich.
Debian warnt sogar davor, den Algorithmus selber zu implementieren, weil sich die Regeln durchaus auch ändern können (und es mit der Einführung der Tilde auch schon getan haben).

## Quitz

Bringe folgende Versionsnummer in die richtige Reihenfolge:

- 1
- 0:2
- 0.9
- 1:1
- 1-1
- 1alpha-0.1
- 1~beta-1
- 1~rc-1
- 2+really1-1
- 1-1+b1
- 1+deb10u1-1
- 1-1.42.202006110951
- 1-1A~4.4.4.202006110951

Für die Auflösung: `~phahn/bin/deb-ver-comp 1 0:2 0.9 1:1 1-1 1alpha-0.1 1~beta-1 1~rc1-1 2+really1-1 1-1+b1 1+deb10u1-1 1-1.42.202006110951 1-1A~4.4.4.202006110951`

{% include abbreviations.md %}
