---
title: 'Automagischer Login an Captive Portals'
date: '2019-01-25T17:16:04+01:00'
layout: post
categories: linux
---

Wer wie ich öfters mit der Bahn unterwegs ist weiß, dass in einigen Zügen die Bahn inzwischen WLAN anbietet:
[WIFI@DB](https://www.dbregio.de/db_regio/view/zukunft/wlan.shtml).
Die Bahn greift dabei auf die Technologie des Unternehmens [Hotsplots](https://www.hotsplots.de/) zu:
Nachdem man eine IP-Adresse per DHCP bekommen hat kann man noch nicht direkt los legen, sondern muss auf einer Portalseite zunächst die AGB akzeptieren.
Dazu braucht man einen Browser.
Das nervt mich, wenn ich das jedesmal wieder von Hand machen muss.

Technisch funktionieren diese sogenannten [Captive Portals](https://de.wikipedia.org/wiki/Captive_Portal) (CP) so:

1. Per DHCP wird einem ein DNS-Server von Hotsplots zugewiesen.
2. Dieser beantwortet einfach alle Anfragen mit der festen IP-Adresse des CP-Servers.
3. Erst wenn man dort die AGB akzeptiert hat, wird die eigene MAC-Adresse im DNS freigeschaltet:
    1. Erst dann liefert der DNS die richtige Adresse.
    2. Erst dann lässt einen die Firewall mit anderen Rechnern außerhalb des WLAN-Netzes kommunizieren.

Da immer mehr Webseiten nur noch per `https://` erreichbar sind, akzeptieren viele Browser die IP-Adresse des CP-Servers nicht, weil der natürlich kein gültiges Zertifikat für die ursprünglich vom Benutzer eingegebene Adresse vorweisen kann.
Man muss also noch eine der wenigen Seiten ansurfen, die noch per `http://` erreichbar sind.
Das führt dann zu solchen Blüten wie [NeverSSL](http://neverssl.com/) oder [RFC7710](https://tools.ietf.org/html/rfc7710).

Das explizite Anmelden per Browser ist mühsam.
Zumindest kann man unter Linux dem [NetworkManager](https://wiki.gnome.org/Projects/NetworkManager) (NM) beibringen, das zu vereinfachen:
Mann kann dort eine [URL konfigurieren](https://jlk.fjfi.cvut.cz/arch/manpages/man/NetworkManager.conf.5#CONNECTIVITY_SECTION), die der NM probiert zu erreichen.
Falls das nicht gelingt zeigt er direkt ein Popup-Dialog an, über den man den Browser starten kann.
(Leider wird die Meldung viel zu kurz angezeigt, so dass ich selten schnell genug bin, den Knopf zu drücken;
ggf. aber auch nur mein 43+ Problem 😉 )

Dazu legt man z.B. folgende Konfigurationsdatei `/etc/NetworkManager/conf.d/10-connectivity.conf` an:
```ini
[connectivity]
uri=http://network-test.debian.org/nm
response=NetworkManager is online
interval=300
```

Mann kann auch andere Seiten konfigurieren, denn der Betreiber dieser Seite kann einen damit natürlich sehr einfach tracken.
Der Dienst muss nur eine simple Seite von Typ `text/plain` ausliefern, deren Inhalt unter `response` angegeben wird.
Standardmäßig verwendet NM einen Dienst von [KDE](http://networkcheck.kde.org/), der ein einfaches `OK` liefert.
Debian patched da aber seinen [eigenen Dienst](http://network-test.debian.org/nm) rein.

Um das ganze weiter zu automatisieren kann man natürlich auch das Verhalten eines Browsers in ein kleines Python-Programm packen.
Die Leute von _Hotsplots_ sind aber nicht ganz doof und haben sich da so manche Falls ausgedacht:

1. Zunächst darf man einer Reihe von Redirects folgen, bis man dann irgendwann auf der Seite mit den AGB landet.
2. Da muss man dann diese über eine Checkbox akzeptieren und einen Button clicken.
3. Anschließend bekommt man eine *unvollständige* HTML-Seite, die über einen Meta-Eintrag `Content-Refresh` eine abschließende URL enthält.
4. Erst wenn man diese URL angesurft hat, ist der Login abgeschlossen.

Interessierte finden mein Skript unter `~phahn/misc/Ansible/files/NetworkManager/50http-captive-portal`.
Ihr braucht neben Python auch noch die Module `lxml` und `requests`.
Verlinkt habe ich es nach `/etc/NetworkManager/dispatcher.d/50http-captive-portal`, aber beim Aufruf geht derzeit leider noch etwas schief, so dass ich es manuell starten muss:
```bash
/etc/NetworkManager/dispatcher.d/50http-captive-portal wlp5s0 up
echo $?
```
Wer dazu sachdienliche Hinweise hat oder Verbesserungsvorschläge für das Skript hat, darf mich gerne kontaktieren.

Disclaimer: Wahrscheinlich steht in dern AGB irgendwo drin, dass man das nicht automatisieren darf, also "use at your own risk".

{% include abbreviations.md %}
