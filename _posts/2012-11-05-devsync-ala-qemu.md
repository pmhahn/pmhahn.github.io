---
title: 'devsync ala QEmu'
date: '2012-11-05T18:35:43+01:00'
layout: post
categories: virt
---

Das `devsync`-Programm aus dem *toolshed* ermöglicht es ja, Dateien vom lokalen System in eine VM zu synchronisieren, um darin dann den Bauvorgang anzustoßen.  
Mit QEmu/KVM auf dem lokalen System geht es übrigens noch ein bisschn besser, denn per [9p](http://wiki.qemu.org/Documentation/9psetup) (vom Betriebssystem [Plan 9](http://de.wikipedia.org/wiki/Plan_9_%28Betriebssystem%29)) kann man ein lokales Verzeichnis auch direkt innerhalb einer **lokalen** QEmu-VM einbinden, je nach Bedarf nur-lesend oder auch durchlässig in beide Richtungen.

`virsh edit "$VM"` aufrufen und ein neues Gerät innerhalb von `/domain/devices/` hinzufügen:

```xml
<filesystem type="mount" accessmode="squash">
  <source dir="/home/phahn/GIT/branches/ucs-3.1/ucs"/>
  <target dir="ucs-3.1"/>
  <readonly/>
</filesystem>
```

Bei `/domain/devices/filesystem/target/@dir` kann man irgendeine Zeichenkette angeben; sie dient lediglich als Handle, um das Dateisystem innerhalb der VM eindeutig zu bezeichnen. Dort kann man es dann folgendrmaßen einbinden:

```bash
mount -t 9p -o ro,trans=virtio,version=9p2000.L ucs-3.1 /mnt
```

Daneben kann QEmu auch noch so ein paar andere ähnliche Dinge wie z.B. on-the-fly aus einem lokalen Verzeichnis ein virtuelles VFAT-Dateisystem zu machen, daß die VM über ein reguläres Blockdevice ansprechen kann.
