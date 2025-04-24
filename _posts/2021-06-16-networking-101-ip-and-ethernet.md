---
title: 'Networking 101: IP and Ethernet'
date: '2021-06-16T08:33:24+02:00'
layout: post
categories: linux
---

Aus gegebenen Anlass, weil scheinbar die Basics fehlen:

Der Netzwerk-Stack wird gerne in Schichten aufgeteilt, beim [OSI-Modell](https://de.wikipedia.org/wiki/OSI-Modell) sind es 7:

1. Bitübertaging (Physical)
2. Sicherung (Data Link) → **Ethernet**
3. Vermittlung-/Paket /Networking) → **IP**
4. Transport (Transport) → **TCP**
5. Sitzung (Session)
6. Darstellung (Presentation
7. Anwendung (Application)

Relevant für die heutige Nachhilfestunde sind nur **IP(v4)** und **Ethernet**.

Das Schichtenmodell dient der Vereinfachung und Strukturierung.
Betrachtet man eine Schichte, regelt diese die Kommunikation auf einer bestimmten Ebene.
Dazu wird dort ein Protokoll definiert, was diesen Ablauf bestimmt.
Nur für die **Umsetzung** dieser Kommunikation wird auf die **direkt darunter liegende Ebene** zugegriffen;
ansonsten bleiben diese Details außen vor und werde nicht *nach oben* durchgereicht.

## Internet Protocol IPv4

Über das *Domain Name System* (DNS) werden Rechnernamen in IP-Adressen aufgelöst.
Die Kommunikation darüber bestimmt das **Routing**:

```console
# ip --color route show
default via 10.200.17.1 dev eth0 onlink
172.17.0.0/16 dev docker0 proto kernel scope link src 172.17.42.1 linkdown
10.200.17.0/24 dev eth0 proto kernel scope link src 10.200.17.38
```

Hier gibt es 2 Arten von Einträgen:

1. `10.200.17.0/24 dev eth0 … src 10.200.17.38` besagt, dass sich dieser Rechner mit der Adresse `10.200.17.38` den IP-Adressbereich `10.200.17.0/24` mit anderen Rechnern teilt.
   Diese sind **direkt** erreichbar, weshalb für die weitere Kommunikation auf das in diesem Fall darunter liegende Ethernet-Protokoll zurückgegriffen werden muss.
2. `default via 10.200.17.1` dagegen besagt, da für alle anderen IP-Adressen, für die es explizit keine andere Regel gibt, die Pakete an den Stellvertreter (Gateway/Router) `10.200.17.1` zu senden sind.
   Um diesen zu erreich muss dessen Adresse wiederum rekursiv aufgelöst werden:
   In diesem Fall gilt die Regel für `10.200.17.0/24`, die eben besagt, dass dieser direkt im lokalen Segment hängt und damit über das darunter liegende Ethernet-Protokoll erreicht werden kann.

## Ethernet Protokoll

Um nun ein IP-Paket per Ethernet zuzustellen, wird das IP-Paket in ein Ethernet-Frame verpackt.
Dieses bekommt die Ethernet-MAC-Adresse des Emfänger:

- Für ein lokalel Paket direkt die Adresse des anderen lokalen Rechners.
- Alle anderen Pakete wird die Adresse des Gateways genutzt, der sich dann um die weitere Weiterleitung kümmern muss.

Um an die MAC-Adresse des Empfänger zu kommen wird ein *Address Resolution Protocol* (ARP)-Paket per Ethernet-Broadcast an alle Rechner im lokalen Segment geschikt.
Dieses Fragt nach der MAC-Adresse, die zu der Ziel-IP-Adresse gehört.
Diese Anfrage wird von allen Rechnern empfangen, aber nur derjenige Rechner, der lokal diese IP-Adresse hat, antwortet dann mit seiner MAC-Adresse.

Nun kennt der sendende Rechner die MAC-Adresse und kann das IP-Paket entsprechend verpacken.
Aus Performancegründen wird die Zuordnung von IP-Adresse zu MAC-Adresse zwischengespeichert, denn ansonsten müsste das für jedes einzelne IP-Paket erneut passieren:

```console
# ip --color neigh show
192.168.0.250 dev eth0 lladdr 18:e8:29:23:f2:b0 STALE
192.168.0.228 dev eth0 lladdr 52:54:00:a0:ea:d6 STALE
192.168.0.124 dev eth0 lladdr 52:54:00:f4:fb:af REACHABLE
```

## Bridge

Früher™ war Ethernet eine einzige große Broadcast-Domäne, d.h. ein Ethernet-Frame war ein Rechner gesendet hat, wurde **immer** von allen anderen Rechnern empfangen, egal ob es für diese bestimmt war oder nicht (Nicht für sie bestimmte Pakete mussten sie also herausfiltern).
Kombiniert mit der Tatsache, dass das Übertragungsmedium immer nur von einem Sender gleichzeitig genutzt werden kann, führt das zu Kollisionen und senkt die zur Verfügung stehende Bandbreite dramatisch je mehr Sender es gibt.

Deswegen verwendet man heutzutage Switches, die die große Broadcast-Domäne in viele kleine Aufteilt:
Jeder Rechner hat seinen eigenen Anschluss und teilt sich dieses Kabel nur mit dem Switch.
Dieser erbt damit die Aufgabe, Kollisionen anderweitig zu verhindern:
Senden tatsächliche mehrere Rechner gleichzeitig Pakete an einen Empfänger, muss der Switch diese nun zwischenspeichern und statt dessen dann der reihe nach ausliefern.

Damit dies effizient erfolgen kann lernt der Switch, über welches Kabel er welche MAC-Adressen erreichen kann:
Nur dadurch kann er Pakete gezielt auf einen Port begrenzen, denn ansonsten müsste er das Ethernet-Paket auf alle seine Ports duplizieren, was die Changce auf Kollisionen natürlich deutlich erhöht (die gibt es zwar nicht, aber das Paket müsste ja trotzdem zwischengespeichert und mehrfach versendet werden).

Der Machanismus dafür ist recht einfach:
Immer wenn ein Rechner ein Paket **versendet**, steht im Ethernet-Frame seine **Absenderadresse**.
Diese assoziiert der Switch mit dem Port, über den das Paket **empfangen** wurde.
Beim nächsten Paket **an** diese Adresse weiß dann der Switch, das dieses Paket **nur** über diesen Port gesendet werden muss.

Nur wenn der Switch die Adresse noch nicht gelernt hat, wird das Paket in *guter alter Tradition* über **alle** Ports versendet – mit all den Nachteilen von Kollision von oben…

Für virtuelle Maschinen wird das in Software realisiert:
```console
# brctl show
bridge name     bridge id               STP enabled     interfaces
docker0         8000.0242972c1aa8       no
eth0            8000.ac1f6bbc0b96       no              enp96s0f0
```

Die Bridge `eth0` hat (derzeit) 2 Port, an dem einerseite das physikalische Interface `enp96s0f0` hängt, andererseite eine VM mit deren Interface `vnet0`.

Die gelernten MAC-Adressen lassen sich ebenso auslesen:

```console
# brctl showmacs eth0
port no mac addr                is local?       ageing timer
  1     00:00:00:00:11:22       no                 4.04
```

## Beispiel

Meine VM mit IP-Adresse `10.200.17.38/24` auf `lagan 192.168.0.49` will mit `omar 192.168.0.10` kommunizieren, also `10.200.17.38 → 192.168.0.10`.

1. In der Routing-Tabelle meiner VM gibt es keine Eintragung für `192.168.0.10` noch für `192.168.0.0/24`
2. Also wird entschieden, das Paket an das default-Gateway `10.200.17.1` zu shicken.
3. Dazu wird per ARQ-Request die Adresse `10.200.17.1` aufgelöst
4. Diese gehört `lerberg`, der mit `80:2a:a8:df:d0:8d` antwortet
5. Das IP-Paket an `192.168.0.10` wird also in ein Ethernet-Frame an `80:2a:a8:df:d0:8d` verpackt und per `eth0` auf die Reise geschickt
6. Dort kommt es aus dem virtuellen Interface `vnet0` der VM an.
   Dieses Steckt in der Bridge `eth0`, von der es weitergeleitet wird.
7. Das Ethernet-Paket verlässt `lagan` über `enp96s0f0` in Richtung `lerberg`
8. Die Netzwerkkarte von `lerberg` fühlt sich angesprochen, weil das Paket an ihr MAC-Adresse adressiert ist
9. `Lerberg` konsultiert seine konfigurierten IP-Adressen und sieht, dass das Paket nicht an ihn persönlich gerichtet ist.
10. Anhand **seiner** Routing-Informationen entscheidet er, das Paket weiter in Richtung `192.168.0.10` zu schicken.
11. Da er selber die IP-Adresse `192.168.0.240` in diesem Netz hat braucht, kann er das Paket selber direkt per Ethernet zustellen.
12. Dazu sellt er einen ARP-Request nach `192.168.0.10`, was `omar` mit `52:54:00:63:dd:87` beantwortet.
13. `lerberg` verpackt erneut das IP-Paket in ein Ethernet-Frame an `52:54:00:63:dd:87` und schickt es raus.
14. `omar` fühlt sich angesprochen und nimmt das Paket an.
15. Da `omar` die IP-Adresse `192.168.0.10` gehört, verarbeitet er das IP-Paket dann weiter.

Für das Antwortpaket passiert das selbe wieder:
Von `omar` per routing an `lerberg` weiter zu `lagan`’s bridge und von dort aus zur VM.

## Besonderheiten

Unser KNUT-Netz hat ein paar Besonderheiten:
Obwohl jedes Mitarbeity seinen iegenen IP-Adressbereich hat, befinden sich alle Rechner und VM in der **selben** Ethernet-Domäne.
Das führt teilweise zu seltsamen Paket-Routing:

- Kommuniziert meine VM `10.200.17.38` auf `lagan` mit `lagan = 192.168.0.49` selbst, macht auch dieses Paket einen Umweg über `lerberg`, obwohl diese IP-Addresse auf `lagan`’s Bridge `eth0` konfiguriert ist.
  Die **VM** entscheidet sich für Routing an `lerberg` und trägt **dessen** MAC-Adresse in das Ethernet-Frame ein.
  Damit passiert dieses die Bridge von `lagan` transparent und erst auf dem Rückweg von `lerberg` steht dann die richtige MAC-Adresse `ac:1f:6b:bc:0b:96` von `lagan` drin, so dass er sich erst dann angesprochen fühlt.
- Selbiges gilt auch für 2 VMs aus unterschiedlichen IP-Adressbereichen.
- Zwei VMs auf dem selben IP-Adressbreich, die zudem auch auf dem selben KVM-Host laufen, nutzen eben kein Routing und kommunizieren direkt per Ethernet, so dass diese Pakete nie den KVM-Server Richtung `lerberg` verlassen müssen.

Das ist so gewollt denn in einem *echten* Netzwerk würde man die Bereiche z.B. durch eine Firewall voneinander abschotten wollen.

Auf der anderen Seite kann man das aber auch zu seinem Vorteil ausnutzen und z.B. weitere Routen ergänzen:
```console
# ip route add 192.168.0.0/24 dev eth0
```
Damit weiß die VM, dass sie auch mit Host aus dem KNUT-Netz direkt verbunden ist und mit diesen direkt per Ethernet sprechen kann, ohne den Umweg über `lerberg` nehmen zu müssen.

Aber:
Die VM hat selber ja keine IP-Adresse in diesem Bereich und wir also weiterhin `10.200.17.34` als Absender eintragen.
`omar` wird also beim Antwortpaket feststellen, dass in **seiner** Routing-Tabelle kein eintrag für `10.200.17.0/24` vorhanden ist und das Antwortpaket deswegen weiterhin über `lerberg` routen.
Damit hat man also nur den **Hinweg** verkürzt, nicht aber den **Rückweg**.
Dafür müsste man auf `omar` (und ggf. auf allen anderen Rechner auch) auch dort passende Routing-Einträge für `10.200.17.0/24 → eth0` ergänzen.

## VM bridging

Wie oben beschrieben baut der Linux-Kernel einen Switch in Software nach.
Dieser Mechanismus hat einen gravierenden Nachteil:
Normalerweise reagiert die Netzwerkkarte nur auf *ihre* MAC-Aresse und filtert alle nicht an sie gerichtetet Pakete *in Hardware* heraus.
Auf einem Server mit einer Bridge, die z.B. für Virtualisierung genutzt wird, muss die Netzwerkkarte aber eben nicht mehr nur die Pakete entgegen nehmen, die direkt an den Host gerichtet sind, sondern auch die Pakete, die für die VMs gedacht sind.
Diese haben alle eigene MAC-Adressem (bei Qemu mit dem Prefix `52:54:00`).
Das Host-Interface muss deshalb im sog. `promiscuous mode` laufen, wodurch **alle** Ethernet-Pakete angekommen werden.
Das sind i.d.R. viel zu viele weil darunter eben auch viele Pakete sind, die weder an diesen Host noch eine seiner VMs gerichtet sind.
Diese Paketflut muss nun der Linux-Kernel in Software bewältigen, was CPU-Zeit bindet.
Zusätzlich darf man nicht vergessen, das Switches eben erst dann effektiv funktionieren, wenn sie MAC-Adressen gelernt und damit das Broadcasting vermeiden können.
Selbiges gilt eben auch hier für jede Linux-Bridge.

Weiterhin können Bridges zu einem Sicherheitsproblem werden:
Innerhalb einer VM kann man den ARP-Request umgehen und selber einen solchen Eintrag erstellen, was dann die direkte Kommunikation mit dem Host ermöglicht, was u.a. bei öffentlichen Cloud-Provider nicht gewünscht ist.

Aus dem Grund wir neuerdings [Macvlan](https://hicu.be/bridge-vs-macvlan) statt Bridging empfohlen:

- Man spart sich das Einrichten einer Bridge, weil sich `Macvlan` im Gegendatz dazu direkt auf jedes existierende Interface aufschalten lässt.
- Es ist Performanter.
- Es ist sicherer, weil es die Kommunikation zwischen VM und Host garantiert unterbindet.
- Moderne Netzwerkkarten bieten dafür Unterstützung in Hardware an, was die Host-CPU entlastet.

Und bevor jemand fragt:
`Macvlan` lässt sich nicht mit Birdges kombinieren, also entweder-oder.

## Anhang

[„iproute2“ statt ‚ifconfig,route,vconfig,ifenslave,brctl‘]({% post_url 2016-04-07-iproute2-statt-ifconfigroutevconfigifenslavebrctl %})

```console
# hostname
omar
# ip --color link show dev eth0
2: eth0:  mtu 1500 qdisc pfifo_fast state UP mode DEFAULT group default qlen 1000
    link/ether 52:54:00:63:dd:87 brd ff:ff:ff:ff:ff:ff

# hostname
vm
# ip --color addr show dev eth0
5: eth0:  mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 52:54:00:bc:0b:96 brd ff:ff:ff:ff:ff:ff
    inet 10.200.17.38/24 brd 10.200.17.255 scope global eth0
       valid_lft forever preferred_lft forever
    inet6 fe80::5254:00ff:febc:b96/64 scope link
       valid_lft forever preferred_lft forever
```

{% include abbreviations.md %}
