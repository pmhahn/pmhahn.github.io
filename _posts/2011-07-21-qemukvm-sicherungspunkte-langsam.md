---
title: 'Qemu/KVM Sicherungspunkte langsam'
date: '2011-07-21T14:33:36+02:00'
layout: post
categories: virt
---

Leider ist das Erstellen, Zurückspielen und Löschen von Sicherungspunkte sowohl in unserer Testumgebung (und leider auch bei Kunde) mit **qemu-0.14.0** extrem langsam: Standardmäßig verwendet KVM als Cache-Strategie **writetrough**, was dafür sorgt, das Änderungen sofort auf die Festplatte zurückgeschrieben werden, um Datenverlust bei Stromausfall vorzubeugen. Bei den Dateioperationen für Sicherungspunkte müssen an sehr vielen Stellen Änderungen gemacht werden, was dann zu der schlechten Performance führt, insbesondere wenn auch noch NFS verwendet wird.

Als Workaround kann man das Cache-Verhalten ändern, was zwar Performance bringt, aber auch zu Datenverlust (z.B. bei Stromausfall) führen kann: Dazu per `virsh edit $VM` die VM-Beschreibung editieren und in der Zeile `<driver name="qemu" type="qcow2"/>` ein cache="none"` hinzufügen. Anschließend muß die VM per UVMM bzw. `virsh destroy $VM; virsh start $VM` neu gestartet werden; ein Reboot des Gasts reicht **nicht**, weil der qemu-Prozeß neu gestartet werden muß!

Aufpassen muß man auch bei vorhandenen Sicherungspunkten, denn beim Wiederherstellen dieser wird auch die alte Konfiguration mit der alten Cache-Strategie wiederhergestellt. Alternativ kann man die Dateien unter `/var/lib/libvirt/qemu/snapshot/$VM/*.xml` auch entsprechen anpassen, allerdings muß danach `libvirtd` per `systemd restart libvirtd` neu gestartet werden, weil diese Dateien nur einmalig bei dessen Start eingelesen werden.

Eine bessere Lösung liefert der Patch an [Bug #22231](https://forge.univention.org/bugzilla/show_bug.cgi?id=22231) für **Qemu**, der nur für das Schreiben der Sicherungspunktinformationen das Cache-Verhalten innerhalb von **qemu** anpasst und damit Sicherheit und Schnelligkeit kombiniert.
