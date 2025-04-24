---
title: 'TotW: qemu-guest-agent'
date: '2018-03-23T14:20:26+01:00'
layout: post
categories: virt
tags: totw
---

Wer kennt das Problem nicht: Die VM wurde über Nacht auf die Festplatte suspendiert und am nächste Morgen geht die Uhr der VM falsch.

Mit `tinker panic 0` in `/etc/ntpd.conf` korrigiert sich das zwar irgendwann, aber nervig ist es trotzdem.

libvirt bietet die Möglichkeit, innerhalb der VM den sog. **qemu-guest-agent** zu starten, der es dann libvirt erlaubt, von extern verschiedenen Dinge anzustoßen:

1. das Stellen der Uhr: `domtime --now $VM`
2. das Herunterfahren (als Alternative zu einem ACPI-Power-Button Event): `shutdown --mode=agent $VM`
3. die IP-Adresse(n) abfragen: `domifaddr --source agent $VM`
4. Informationen über gemountete Dateisysteme abfragen: `domfsinfo $VM`
5. und [mehr](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/virtualization_deployment_and_administration_guide/sect-using_the_qemu_guest_virtual_machine_agent_protocol_cli-libvirt_commands)

Dazu braucht libvirt zunächst einen Kanal in die VM: Mit `virsh edit "$VM"` fügt man innerhalb vom Abschnitt `<devices>..</devices>` folgendes Fragment hinzu:
```xml
 <channel type="unix">
  <source mode="bind"/>
  <target type="virtio" name="org.qemu.guest_agent.0"/>
 </channel>
```

Anschließend installiert man innerhalb der VM den Agent mit `apt install qemu-guest-agent`.

Dann kann man per `virsh domtime --now "$VM"` z.B. die Uhr stellen.

Schade nur, dass UVMM und UCS das nicht out-of-the-box können.
