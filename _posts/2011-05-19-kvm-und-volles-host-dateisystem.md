---
title: 'KVM und volles Host-Dateisystem'
date: '2011-05-19T08:11:23+02:00'
layout: post
categories: virt
---

Bei KVM wird fast immer das Qcow2-Dateiformat für die Speicherung des Festplattenimmages verwendet.
Dieses Format belegt zunächst wenig Speicher, wächst aber bei steigender Nutzung durch das Gast-Betriebssystem bis zur angegebenen Höchstgrenze an.
Durch Sicherungspunkte kann so eine Datei aber auch deutlich größer sein als die nominal vorgegebene Größe.
Dies kann u.a. dazu führen, das im Betrieb das Dateisystem des Host-Systems voll läuft und die Qcow2-Datei nicht weiter anwachsen kann.
In diesem Fall pausieren die betroffenen KVM-Instanzen, so daß es hier nicht zu Datenverlust kommt, solange das nicht von Hand per [`/domain/devices/disk/driver/@error_policy="stop|ignore|enospace"`](http://libvirt.org/formatdomain.html#elementsDisks) geändert wurde.
Nachdem man wieder freien Speicherplatz geschaffen hat, muß die VM per Kommandozeilentool fortgesetzt werden:
`sudo virsh qemu-monitor-command "$VM" cont`.

{% include abbreviations.md %}
