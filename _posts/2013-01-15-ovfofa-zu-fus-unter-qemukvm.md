---
title: 'OVF/OFA zu Fuß unter QEMU/KVM'
date: '2013-01-15T09:29:09+01:00'
layout: post
categories: virt
---

Open Virtualization Format (OVF) ist eine Spezifikation, die es ermöglicht, Virtuelle Maschinen leicht in die eigene Virtualisierungslösung wie QEMU, Xen, VirtualBox oder VMWare zu importieren bzw. zwischen diesen auszutauschen.
In der einfachsten Form besteht so eine VM aus einem Verzeichnis mit 3 Dateien:

1. Einer Image-Datei, die das Festplattenabbild enthält und i.d.R. das VMDK-Format benutzt.
2. Ein XML-Datei, die CPU-Anzahl, RAM-Größe und ähnliches festlegt.
3. Ein Manifest-Datei, die die anderen Dateien referenziert und deren SHA1-Prüfsummen enthält.

Neben der Unterstützung von Zertifikate für die digitale Signierung kann ein ein OVF-Paket auch mehrere VMs enthalten, so daß komplette Umgebungen bestehend aus mehreren Maschinen ausgetauscht werden können.

Da die Handhabung von mehreren Datein für den Austausch eher umständlich ist, können dieses Dateien in einer einzelnen OVA-Datei zusammengefasst werden.
(Eine OVA-Datei ist lediglich ein TAR-Archiv der OVF-Dateien)

OVA-Archive können derzeit nicht direkt in QEMU/KVM importiert werden, es existietr aber ein auf libvirt-aufbauendes Projekt, das dieses ermöglicht.
Alternativ kann man das auch per Hand machen, da QEMU/KVM eigentlich auch alle Geräte unterstützt, die auch von VMWare benutzt werden.
Einziges Problem ist derzeit des LSILogic-SCSI-Controller, der mit QEMU-1.1 noch ein eigenes BIOS benötigt, um davon booten zu können:

1. OVA-Archiv mit `tar xf "$VM.ofa"` entpacken
2. Aus der Datei `$VM.ovf` folgende Informationen extrahieren:
    1. Number of Virtual CPUs
    2. Memory Size
3. Daraus eine libvirt-XML-Datei erstellen:
    ```xml
    <domain type="kmv">
        <name>NAME</name>
        <memory unit="KiB">SIZE</memory>
        <vcpu>CPUS</vcpu>
        <os>
            <type arch="x86_64" machine="pc">hvm</type>
        </os>
        <features>
            <acpi/><apic/><pae/>
        </features>
        <devices>
            <disk type="file" device="disk">
                <driver name="qemu" type="vmdk">
                <source file="FILE.vmdk"/>
                <target dev="sda" bus="scsi"/>
            </disk>
            <controller type="scsi" index="0″ model="lsilogic"/>
            <interface type="bridge">
                <source bridge="eth0″/>
                <model type="pcnet"/>
            </interface>
            <input type="tabet" bus="usb"/>
            <video>
                <model type="vmware" vram="9216″/>
            </video>
        </devices>
        <qemu:commandline xmlns:qemu="http://libvirt.org/schemas/domain/qemu/1.0">
            <qemu:arg value="-option-rom"/>
            <qemu:arg value="/var/lib/libvirt/images/8xx_64.rom"/>
        </qemu:commandline>
    </domain>
    ```
4. Von der www.lsi.com-Seite die Datei `lsi_bios.zip` herunterladen und daraus die Datei `8xx_64.rom` extrahieren und verfügbar machen:
    `unzip -d /var/lib/libvirt/images/ lsi_bios.zip 8xx_64.rom`
5. `virsh define "$VM.xml"`
6. `virsh start "$VM"`

Bekannte Probleme:

- Nicht alle VMDK-Formate können von qemu-1.1 geschrieben werden:
  > VMDK: can’t write to allocated cluster for streamOptimized.

  Als Work-Around muß man die Datei in ein anderes Format (qcow2, qed, raw) mit `qemu-img convert -f vmdk -O qcow2 "$DISK.vmdk" "$DISK.qcow2"` konvertieren oder einfach eine qcow2-Datei über die vmdk-Datei legen `qemu-img create -f qcow2 -b "$DISK.vmdk" "$DISK.qcow2"`.
  Entspechend muß in der libvirt-XML-Datei dann auch `vmdk` durch `qcow2` ersetzt werden.
- Das Booten von SCSI erfodert mit qemu-1.x zwingend das LSI-BIOS.
- Wenn kein DHCP-Server konfiguriert ist, kann man das Interface `virbr0` statt `eth0` verwenden, was einem lokalen Netz mit NAT und DHCP entspricht.
  Das muß ggf. noch mit `virsh net-start default` gestartet werden.

{% include abbreviations.md %}
