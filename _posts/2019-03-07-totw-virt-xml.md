---
title: 'TotW: virt-xml'
date: '2019-03-07T17:23:13+01:00'
layout: post
categories: virt
tags: totw
---

Q: Wie kann ich XML-Definition von VMs anpassen?

A: `virsh edit` kennen vermutlich inzwischen viele, aber das erfordert Handarbeit und lässt sich nicht automatisieren.

Alternativ kann man das Debian-Paket `virtinst` installieren, was dann u.a. `virt-xml` mitbringt, mit dem man per Kommandozeile VMs modifizieren kann:

```bash
HOST="qemu+ssh://${USER}@lattjo.knut.univention.de/system"
vm="${USER}_pt44-test"
vxml () { virt-xml --quiet --connect "$HOST" "$@" "$vm"; }
vxml --edit --memory 2048
vxml --edit --vcpus 2
vxml --edit --features hyperv_relaxed="on"
vxml --edit --features hyperv_vapic="on"
vxml --edit --features hyperv_spinlocks="on", hyperv_spinlocks_retries="8191"
vxml --edit --cpu host-model,clearxml="yes"
vxml --edit --clock hypervclock_present="yes"
vxml --edit --clock hpet_present="no"
vxml --edit --clock pit_present="yes",pit_tickpolicy="delay"
vxml --edit --clock rtc_present="yes",rtc_tickpolicy="catchup"
vxml --edit --video model="vga"
vxml --add-device --watchdog model="i6300esb",action="reset"
vxml --add-device --rng /dev/urandom
vxml --add-device --channel char_type="unix",mode="bind",target_type="virtio",name="org.qemu.guest_agent.0"
vxml --add-device --console char_type="pty"

vxml --add-device --disk path="/var/lib/libvirt/images/${vm}-2.qcow2″,size=100,cache="unsafe",bus="scsi",discard="unmap",driver_name="qemu",driver_type="qcow2"
vxml --edit type=scsi --controller type="scsi",model="virtio-scsi"
```

PS: Zumindest bei mit hat das Anlegen eines neues Storage Volumes über einen Storage Pool zu einem Traceback geführt.

{% include abbreviations.md %}
