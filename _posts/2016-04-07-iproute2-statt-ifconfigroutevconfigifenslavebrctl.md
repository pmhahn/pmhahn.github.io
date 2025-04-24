---
title: '"iproute2" statt "ifconfig,route,vconfig,ifenslave,brctl"'
date: '2016-04-07T17:00:58+02:00'
layout: post
categories: linux
---

Mit `iproute2` hat Linux schon lange einen Nachfolger für die althergebrachten Tools wie `ifconfig` und `route`.
Die gibt es zwar immer noch, aber für viele neuen Funktionen braucht man dann schon extra Tools wie `bridge-utils`, `vlan`, `ifenslave` (für Bonding).
Der exakte Umfang hängt allerdings von der Kernel-Version und von der Paket-Version von `iproute2` ab.

Mit [iproute2](http://baturin.org/docs/iproute2/) gibt es eine sehr schöne Übersicht, was man mit iproute so alles machen kann:

# **Ethernet-Interface** konfigurieren (<del>ifconfig</del>,<del>route</del>)

```bash
ip addr replace 10.200.17.2/24 dev "eth0"
ip link set dev "eth0" up
ip route replace default via 10.200.17.1 dev "eth0"
```
`replace` sorgt dafür, das eine ggf. vorher bereits existierende Adresse bzw. Route ersetzt wird;
`add` würde sich sonst darüber beschweren.
Es lassen sich auch weitere Adressen hinzufügen, ohne das man die früheren virtuellen Interfaces (eth0:1) weiterhin verwenden muss, was bei IPv6 durchaus standard ist:
```bash
ip addr add f001:4dd0:ff00:8c42:ff17::5254:00af:f03c/80 dev "eth0"
```
Da teilen sich mehrer IP-Adressen die selbe Ethernet-MAC-Adresse.
Mit **macvlan** kann man alternativ auch weitere virtuelle Ethernet-Interfaces mit eigenen MAC-Adressen erzeugen, um z.B. Dienste gezielt an eine IP-Adresse binden zu können:
```
ip link add name "eth-ssh" link eth0 type macvlan
ip addr add 10.200.17.3/24 dev "eth-ssh"
ip link set dev "eth-ssh" up
…
ip link del dev "eth-ssh"
```

# **Bridge** einrichten (<del>brctl</del>, <ins>bridge</ins>)
```bash
ip link add name "br-inet" type bridge
ip addr add 10.200.17.2/24 dev "br-inet"
ip link set dev "br-inet" up
ip link set dev eth0 master "br-inet"
ip link set dev eth0 up
ip route replace default via 10.200.17.1 dev "br-inet"
```

# **802.1q VLAN** einrichten (<del>vconfig</del>)
```bash
ip link add name "vlan-phahn17" link eth0 type vlan id 17
ip addr add 10.200.17.2/24 dev "vlan-phahn17"
ip link set dev "vlan-phahn17" up
…
ip link del dev "vlan-phahn17"
```

# **Kanal-Bündelung** (Bonding) einrichten (<del>ifenslave</del>)
```bash
ip link add name "bond-intern" type bond
ip link set dev eth0 master "bond-intern"
echo … >/sys/class/net/"bond-intern"/bonding/…
…
ip link del dev "bond-intern"
```

# **tap**-Interface für Ethernet-Frames anlegen (<del>tunctl</del>,<del>openvpn --mktun</del>)

Das eine Ende ist ein Netzwerk-Interface `tap0`, das andere ein Device-File `/dev/net/tun`, aus dem Ethernet-Frames heraus kommen bzw. geschrieben werden können.
```bash
ip tuntap add dev "tap0" mode tap user phahn group users
ip link set dev "tap0" up
…
ip link del dev "tap0"
```
Alternativ kann man auch **tun**-Interfaces für IP(v4)-Pakete konfigurieren:
Wie `tap`, nur das IP(v4)-Pakete statt Ethernet-Frames verarbeitet werden.
Mit **macvtap** gibt es noch ein zu `tap` kompatible Interfaces für VMs, die aber auch wie `macvlan` direkt eine eigene MAC-Adresse bekommen und so keine explizite Bridge benötigen.
Allerdings kann (Design-bedingt) der Host dann nicht direkt mit den VMs kommunizieren, sondern der (externe) Switch muss die Pakete zurück schicken.
Hintergrund ist, dass die Linux-Bridge eher ineffizient ist, weil das Ethernet-Device dann im promiscuous-Modus laufen muss und damit zu viele Pakete aus dem Netzwerk die Bridge erreichen und damit von der Host-CPU gefiltert werden müssen.
Mit `macvtap` bekommt jede VM seine eigene MAC-Adresse und nur diese werden über das Ethernet-Device an den Switch publiziert, so dass dieser schon die Pakete filtern kann.

# **Virtual-Ethernet-Device** (veth) einrichten

Prinzipiell nur ein Patch-Kabel zwischen zwei Netzwerk-Interfaces, die direkt miteinander verbunden sind, ohne `/dev/net/tun` dazwischen.
Nützlich für Container oder für die direkte Kommunikation zweier lokale Prozesse, die nur per Netzwerk kommunizieren.
```bash
ip link add name "veth-host" type veth peer name "veth-guest"
ip netns add "my-env"
ip link set dev "veth-guest" netns "my-env" name "eth0"
ip netns exec "my-env" ip …
…
ip link del dev "veth-host"
ip netns del "my-env"
```

Netzwerk-Interfaces können einen beliebigen Namen haben:

- maximal 15 Zeichen
- keine Leerzeichen, Tabs, Zeilenumbrüche, Slashes
- nicht `.` oder `..`

{% include abbreviations.md %}
