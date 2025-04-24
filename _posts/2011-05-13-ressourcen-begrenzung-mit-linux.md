---
title: 'Ressourcen-Begrenzung mit Linux'
date: '2011-05-13T08:41:30+02:00'
layout: post
categories: linux virt
excerpt_separator: <!--more-->
---

Solange man genügend Ressourcen hat und der Rechner bzw. die Festplatte nichts zu tun hat, ist es fast immer egal, welche Prozesse genau was machen.
Wehe aber, wenn die Ressourcen nicht für alle gleichzeitig ausreichen und der Kampf darum beginnt, denn dann zählt nur noch jeder Prozeß für sich.
Das kann dazu führen, daß der Benutzer mit 10 Prozessen eben auch die 10-fache Rechenzeit bekommt als der brave Benutzer mit nur einem Prozeß.

<!--more-->

Unter Xen kann man die CPU-Zeit der virtuellen Instanzen per [xm sched-credit](http://wiki.xensource.com/xenwiki/CreditScheduler) limitieren, unter KVM sucht man Vergleichbares zunächst vergebens.
Das ist auch kein Wunder, den die Philosophie der *Kernel-based Virtual Machine* (KVM) ist es ja gerade, die Standard-Funktionen des Linux-Kernel wie (Speicher- und Prozeßverwaltung) zu benutzen, anstatt sie abermals neu umzusetzen, wie es Xen mit seinem Hypervisor tut.

Umgekehrt bedeutet das eben, das die Funktionen zum Beschränken von Ressourcen eben nicht nur auf virtuelle Maschinen beschränkt ist, sondern für alle Prozesse eines Linux-Systems zur Verfügung stehen:
Willkommen bei [cgroups](http://www.serverwatch.com/tutorials/article.php/3920051/Introduction-to-Linux-Cgroups.htm), dem Linux-Kernel-Subsystem für die Ressourcen-Verwaltung.
Grundgedanke dahinter ist es, alle Prozesse bezüglich unterschiedlicher Kategorien zu partitionieren, d.h. bezüglich einer Kategorie (z.B. CPU-Zeit) ist jeder Prozess genau **einer** Gruppe von Prozessen zugehörig, für die gemeinsam ein gewisses Kontingent von Ressourcen zur Verfügung steht.

Ein kleines Beispiel:

```bash
su root -c "aptitude install cpuburn"
su root -c "mount -t cgroup -o cpu cgroup /sys/fs/cgroup ; mkdir -p /sys/fs/cgroup/top/{hi,lo} ; chown -R '$USER' /sys/fs/cgroup/top"
echo 512 >/sys/fs/cgroup/top/hi/cpu.shares
echo 256 >/sys/fs/cgroup/top/lo/cpu.shares
for i in `seq 2`; do burnP6 & echo $! >/sys/fs/cgroup/top/hi/tasks; done
for i in `seq 3`; do burnP6 & echo $! >/sys/fs/cgroup/top/lo/tasks; done
top -u "$USER"
```

liefert dann folgendes Bild:

```
PID  USER     PR NI VIRT RES SHR S %CPU %MEM TIME+  COMMAND
5823 Administ 20 0  100  12  8   R 65   0.0 1:02.93 burnP6
5822 Administ 20 0  100  16  8   R 61   0.0 1:07.32 burnP6
5825 Administ 20 0  100  12  8   R 29   0.0 0:21.06 burnP6
5826 Administ 20 0  100  12  8   R 18   0.0 0:19.36 burnP6
5824 Administ 20 0  100  16  8   R 18   0.0 0:20.94 burnP6
                                   ^^
```

Wichtig ist hier, das obwohl die 5 Prozesse eigentlich die selbe Endlosschleife ausführen, sie unterschiedliche CPU-Zeiten erhalten.
Je nach dem in welcher Ressourcen-Gruppe (*hi* bzw. *low*) sie sind, erhalten sie zusammen von der insgesamt zur Verfügung stehenden CPU-Zeit entweder 512 Anteile oder nur 256 Anteile.
Die ersten beiden Prozesse (5822, 5823) erhalten zusammen etwa doppelt so viel Zeit wie die anderen drei Prozesse zusammen.

Neben der CPU-Zeit-Begrenzung gibt es noch weitere Kategorien, so z.B. auch für die Beschränkung auf verschiedenen CPU-Kerne, das Beschränken des Öffnen von Gerätedateien unter */dev/*, das Beschränken der IO-Bandbreite, das Einfrieren kompletter Gruppen von Prozessen, und einige andere.
[libvirt](http://berrange.com/posts/2009/12/03/using-cgroups-with-libvirt-and-lxckvm-guests-in-fedora-12/ "Using CGroups with libvirt and LXC/KVM guests in Fedora 12") nutzt cgroups berreits, um die CPU-Zeit der virtuellen Instanzen zu beschränken oder den Zugriff von virtuellen Instanzen auf Block-Devices zu regeln.

Das Interessante an cgroups ist, das Kind-Prozesse zunächst immer der gleichen cgroup angehören, wie ihr Vaterprozeß (außer sie werden eben explizit in eine andere Partition verschoben).
Während es durch double-*fork*s möglich ist, seinen Eltern zu entkommen (was z.B. Daemon-Prozesse machen), ist das bei cgroups nicht so.
Da nutzt z.B. auch [systemd](http://freedesktop.org/wiki/Software/systemd) dazu, beim Beenden von Diensten auch alle von diesen geforkten Kindprozesse mit zu beenden.

PS: Das Verzeichnis `/sys/fs/cgroup/` ist hier willkürlich gewählt, man kann auch jedes andere Verzeichnis verwenden.
Bei Debian ist es z.B. `/mnt/cgroups/`, bei RedHat `/dev/cgroups/`, … Es lebe der Standard!

{% include abbreviations.md %}
