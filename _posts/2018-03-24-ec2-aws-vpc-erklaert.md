---
title: 'EC2: AWS-VPC erklärt'
date: '2018-03-24T08:54:20+01:00'
layout: post
categories: network virt
---

Oder: Warum `ucs-test` in EC2 so langsam ist – Folge 1

Ein VPN ist ein abgeschottetes Netzwerk. Amazon nennt es VPC.
In EC2 nutzen wir dafür den IPv4-Adressbereich `10.210.0.0/16`.
Dieser Adressbereich wird öffentlich nicht geroutet, d.h. jeder richtig konfigurierte Router verwirft Pakete aus einem solchen Netz bzw. in ein solches Netz.

In EC2 kann man einer Instanz eine **Elastic IP** geben:
Innerhalb des VPN nutzt die VM weiterhin ihre private Adresse, aber das AWS-IP-Gateway ersetzt die Quell-Adresse durch die öffentliche Adresse und lässt das Paket dann passieren.

Als Alternative gibt es von Amsazon noch ein **NAT Gateway**:
Auch dieses bekommt eine **Elastic IP** und ersetzt für allen ausgehenden Verkehr die private IP-Adresse durch seine IP-Adresse und einen eindeutigen Port.
Dadurch kann das Gateway die Verbindung dem Host eindeutig zuordnen und in Antwortpaketen seine Zieladresse durch die ursprüngliche interne IP-Adresse ersetzen.
Solch ein NAT-Gateway kann man sich als **Service** bei AWS einrichten lassen, was den Vorteil hat, dass es eine von Amazon gemanagte Lösung ist und man selber keine VM pflegen muss.
Diese Lösung hat weiterhin den Vorteil, dass sie sehr gut skaliert (bis zu 45 Gbps) und hochverfügbar ist.

Alternativ kann man man eine **NAT Instanz** betreiben:
Auch das gibt es fertig von Amazon, ist aber nichts anderes als ein Linux-System, in dem Masquerading konfiguriert ist.
Das ist das, was wir bisher gemacht haben:
Innerhalb unseres VPCs lief eine [t1.micro](https://docs.aws.amazon.com/de_de/AWSEC2/latest/UserGuide/concepts_micro_instances.html) Instanz, die allen ausgehenden Verkehr maskiert:

```bash
iptables -t nat -A POSTROUTING -s 10.210.0.0/16 -o eth0 -j MASQUERADE
```

Dieser Instanz-Typ stammt noch aus den Anfangszeiten von AWS und nutzt noch die Xen-Paravirtualisierung.
Sie hat mindestens zwei Nachteile:

1. Ihre Netzwerkperformance ist **Sehr niedrig**, was dazu führt, dass die Bandbreite begrenzt ist.
2. Die CPU-Performance ist durch einen **Leaky-Bucket** begrenzt: Wird wenig CPU-Zeit gebraucht, sammelt sie Guthaben bis zu eine Obergrenze an. Dieses Guthaben kann dann später bei Lastspitzen verbraucht werden. Ist es verbraucht, blockiert die VM bis wieder genügend Guthaben angesammelt wurde.

Starten dann Nachts viele Umgebungen gleichzeitig, ist beides schnell aufgebraucht und alle Instanzen warten vermeintlich auf das langsame Netzwerk.

Damit eine VM im VPN nun eine ausgehende Verbindung ins Internet aufbauen kann, muss man die NAT-Instanz (oder eben ein NAT-Gateway) als Gateway konfigurieren.
Für `ucs-test` machen wir das in `util.sh`:

```bash
GW='10.210.216.13' MDS='169.254.169.254'
echo "supersede routers ${GW};" >> /etc/dhcp/dhclient.conf.local
echo "supersede rfc3442-classless-static-routes 32,${MDS//./,},0,0,0,0,0,${GW//./,};" >> /etc/dhcp/dhclient.conf.local
ip route replace default via "$GW"  # VPN gateway
ip route replace "$MDS" dev eth0  # EC2 meta-data service
ucr set gateway="$GW"
```

Dieser Umweg über die `dhclient.conf.local` ist notwendig, weil die VMs per DHCP ihre Konfiguration beziehen.
Neben der IP-Adresse verteilt der Amazon-DHCP-Server auch die Adresse des Gateways, was hier aber das der Router des VPCs ist, hinter dem es erst dann weiter zum **Internet Gateway** geht.
Da unsere VMs mit ihren privaten Adressen aber NAT brauchen, muss man die Default-Route entsprechend umbiegen.

Wichtig ist aber auch, dass die Amazon **Service-API** über die IP-Adresse `169.254.169.254` direkt erreichbar ist.
Diese API kann die Identität der anfragende VM nur anhand ihrer Ethernet-MAC-Adresse identifizieren:
Richtet man diese spezielle Route nicht ein, so stellt immer die NAT-Instanz stellvertretend für die VMs die Anfrage und alle VMs bekommen die Konfigurationsdaten der NAT-Instanz statt der ihnen zugedachten!

Man kann auch für das Subnetz des VPNs eine korrigierte Routing-Tabelle anlegen, die dann standardmäßig als Default-Route die NAT-Instanz verwendet.
Allerdings muss man dann diese in ein gesondertes Subnetz verschieben und diesem die Standard-Routing-Tabelle geben, denn die NAT-Instanz muss ja weiterhin direkt das Internet erreichen können:
Denn der Router befindet sich **zwischen** der NAT-Instanz und dem Internet-Gateway und er müsste unverschlüsselten Verkehr der VMs zurück an die NAT-Instanz schicken, verschlüsselten Verkehr der NAT-Instanz jedoch ans Internet-Gateway durchlassen.
Da aber lediglich die Ziel-Adresse als Routing-Entscheidung verwendet werden kann und auch der VPN-Verkehr noch die interne IP-Adresse verwendet, ist das nicht ohne gesondertes Subnetz machbar.

Zusätzlich läuft auf dieser EC2-Instanz auch ein **OpenVPN-Server** für unseren **OpenVPN-Tunnel**:
Nur durch diese gelangen wir überhaupt erst in das VPC bei Amazon.
Dazu läuft intern in unserer DMZ die VM **ec2-vpn.pingst.univention.de** mit der internen IP `192.168.5.27`.
Diese baut per **OpenVPN** eine **unverschlüsselte** Verbindung zur NAT-Instanz bei Amazon auf.
Diese Verbindung besteht nur logisch, denn es wird das verbindungslose UDP für den Transport der den OpenVPN-Pakete verwendet.

Technisch ist das durch einen sogenannten *Transit-Tunnel* gelöst:
Beide OpenVPN-Seiten haben jeweils ein **tun**-Interface mit der IP-Adresse `10.211.0.1` bei Amazon bzw. `10.211.0.2` in unserer DMZ.
Per Routing-Tabelle werden die Pakete jeweils an diese lokale Adresse weitergeleitet.

OpenVPN greift diese dann über `/dev/tun0` ab und verpackt die IP-Pakete für die Gegenseite.
Beim Verlassen unserer DMZ passiert es unsere Firewall.
Diese macht auch NAT und ändert die Absenderadresse der OpenVPN-Pakets auf `82.198.197.8`.
Bei Amazon angekommen passiert es dort deren Internet-Gateway, wo die Zieladresse in die private Adresse `10.210.16.13` umgeschrieben wird.
In der dortigen NAT-Instanz angekommen wird das OpenVPN-Paket wieder ausgepackt und an die ursprüngliche IP-Adresse der Ziel-VM weitergeleitet.
Vor dem weiterleiten wird allerdings die Quell-Adresse per Masquerading auf die interne IP-Adresse der NAT-Instanz geändert:

```bash
iptables -t nat -A POSTROUTING -s 10.211.0.0/24 -o eth0 -j MASQUERADE
```

Das wir gemacht, damit die Antwort auf jeden Fall an die NAT-Instanz zurück gesendet wird, auch wenn die VM die NAT-Instanz nicht als Gateway konfiguriert hat.
Das ist zum Beispiel der Fall, **bevor** der obige Hack mit `dhclient.conf.local` ausgeführt worden ist.

<!-- [![EC2-VPN]({{site.url}}/images/AWS-EC2-VPN.svg)]({{site.url}}/AWS-EC2-VPN.svg) -->

Greif man nun aus dem internen Netz **192.168.0.0/24** auf meine VM 10.200.7.14 in EC2 zu, passiert folgendes:

```
traceroute to 10.210.7.14 (10.210.7.14), 30 hops max, 60 byte packets
 1  192.168.0.240  0.362 ms  0.324 ms  0.416 ms
 2  192.168.0.250  0.518 ms  0.392 ms  0.347 ms
 3  192.168.250.250  0.578 ms  0.538 ms  0.581 ms
 4  192.168.5.27  0.822 ms  0.800 ms  0.841 ms
 5  10.211.0.1  32.148 ms  32.126 ms  32.061 ms
 6  10.210.7.14  32.798 ms  32.506 ms  32.468 ms
```

1. Mein Notebook 192.168.0.82 schickt ein Paket an unser lokales Gateway _lerberg_ mit IP `192.168.0.240`.
2. Dieses leitet das Paket zur KNUT-Firewall bei Briteline _rigtig_ mit IP `192.168.0.250` weiter.
3. Dort passiert es die _Firewall_ `192.168.250.250` zur DMZ.
4. Von dort geht das Paket zu unserem OpenVPN-Client `ec2-vpn` mit `192.168.5.27`. Dort wird das Paket durch den *Transit-Tunnel* geleitet. Vom verpacken durch OpenVPN sieht man hier nichts.
5. Beim OpenVPN-Server im Amazon-VPN angekommen wird das Paket entpackt und innerhalb des VPN zugestellt.
6. Schließlich erreicht das Paket die VM.

Wie bekommt man nun mehr Geschwindigkeit bzw. was habe ich heute Nacht geändert?

- Die [t1.micro](https://aws.amazon.com/de/ec2/previous-generation/) Instanz wurde durch eine leistungsfähigere Instanz ersetzen.
- Neben der NAT-Instanz läuft nun im zweiten Subnetz `10.211.0.0/28` auch ein NAT-Gateway, das standardmäßig in der Routing-Tabelle für das alte Subnetz `10.210.0.0/8` konfiguriert wird. Der DHCP-Hack ist damit nicht mehr notwendig für ausgehende Verbindungen.
- Fernen gibt es nun auch ein **IPv6 Egress-Gateway**, welches es erlaubt, dass VMs ausgehende IPv6-Verbindungen aufbauen können. IPv6 ist allerdings erst für die neuere Instanztypen **m4** verfügbar.
- Es existiert ein partieller(nur *maintained*) lokalen Spiegel unseres Download-Servers beim Amazon, den man nutzen kann: `ucr set repository/online/server=http://ucs-updates.s3-website-eu-west-1.amazonaws.com/` Der Spiegel ist transparent und liefert für alle Anfragen, die er nicht beantworten kann, einen HTTP-Redirect auf [updates.software-univention.de](http://updates.software-univention.de/), was mit `apt` auch wunderbar funktioniert. Dieser Spiegel enthält **nur** die statischen `.deb`-Pakete, nicht die dynamischen Daten wie `Packages`-Dateien und Updater-Scripte.

Ich bin gespannt auf eure Rückmeldungen, ob es geholfen hat.

{% include abbreviations.md %}
