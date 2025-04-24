---
title: 'Qcow2 per Network-Block-Device'
date: '2014-01-21T09:23:18+01:00'
layout: post
categories: virt
excerpt_separator: <!--more-->
---

Q: Wie bekommt man Zugriff auf eine 50 GiB großes Qcow2-Datei auf einem anderen KVM-Server, wenn die lokale Platte dafür zu klein ist oder man nicht bereits ist, so lange zu warten, bis die Datei komplett kopiert ist?

A: Man kann die Datei per Network-Block-Device (NBD) nur-lesbar exportieren und auf einem anderen Rechner einbinden:

<!--more-->

Auf dem Quell-KVM-Server (z.B. krus):
```bash
ssh krus
sudo iptables -I INPUT 1 -p tcp --dport 10809 -j ACCEPT
nohup qemu-nbd --port 10809 \
 --read-only \
 --nocache \
 --persistent /var/lib/libvirt/images/lenny-0.qcow2 &
disown
exit
```

Auf dem Ziel-KVM-Server:
```xml
<domain type="kvm">
 …
 <devices>
  <disk type="network" device="disk">
  <driver name="qemu" type="raw"/>
  <source protocol="nbd">
   <host name="192.168.0.238" port="10809"/>
  </source>
  <target dev="vda" bus="virtio"/>
  <readonly/>
 </disk>
 …
 </devices>
</domain>
```

Und weil das noch nicht cool genug ist, kann man auch noch lokal eine Qcow2-Datei darüberlegen, damit das ganze lokal schreibbar wird und Änderungen nicht auf den Server zurückgeschrieben werden:
```bash
qemu-img create -f qcow2 \
 -o backing_fmt=nbd \
 -o backing_file=nbd:krus.knut.univention.de:10809 \
 /var/lib/libvirt/images/lenny-0.qcow2
```

In der XML-Datei muß natürlich dann ganz normal diese Qcow2-Datei eingebunden werden:
```xml
<disk type="file" device="disk">
 <driver name="qemu" type="qcow2" copy_on_read="on"/>
 <source file="/var/lib/libvirt/images/lenny-0.qcow2"/>
 <target dev="vda" bus="virtio"/>
</disk>
```

Das `copy_on_read` sorgt dafür, daß einmal per NBD gelesene Blöcke auch in die lokalen Qcow2-Datei kopiert werden sollen, damit der Block, falls er später nochmals gelesen werden muß, nicht erneut über das Netzwerk gelesen werden muß, sondern schon in der lokalen Datei vorliegt. Geschriebene Blöcke werden natürlich sofort und nur in der lokalen Datei gespeichert, wie man das eben vom Qcow2-Backgind-Datei-Mechanismus bereits kennt.

Damit entsteht im Laufe der Zeit ein mehr-oder-minder vollständiges lokales Abbild der entfernten Datei. Will man wirklich wirklich alle Daten kopieren, kann man anschließend auch dem laufenden Qemu-KVM-Prozess sagen, daß es (in aller Ruhe) die Blöcke der Backing-Datei kopieren soll, so daß anschließend die lokale Qcow2-Datei alle Datenblöcke enthält und man die Verbindung zur ursprünglichen Datei kappen kann:
```bash
virsh blockpull --domain "$domain" \
 --path "/var/lib/libvirt/images/lenny-0.qcow2" \
 --bandwidth 1 # MiB/s
```

Per `virsh blockjob "$domain" "$path"` kann man sich anschließend über den Fortschritt der Kopieraktion informieren.

Ein paar Probleme sollen nicht verheimlicht werden:

- `sudo iptables` ist auf unseren KVM-Servern leider nicht direkt möglich, aber natürlich kann man auch einfach einen der hohen VNC-Ports 5900:5999 missbrauchen: die sind schon erreichbar und man kann sich das `sudo`-sparen.
- `sudo virsh blockpull` und `blockjob` sind leider auch nicht freigeschaltet — "ceterum censeo sudo virsh …"

Interessant ist NBD vor allem deswegen auch, weil das genau der gleiche Mechanismus ist, der für die Migration von Storage-Volumes bei einer Live-Migration genutzt wird, wenn man kein Shared-Storage hat: Der Qemu-Prozess startet intern einfach einen NBD-Server und nutzt dann genau den eben beschriebenen Mechanismus… Ein bisschen wissen darüber wird also auch in der Zukunft nicht schaden.

{% include abbreviations.md %}
