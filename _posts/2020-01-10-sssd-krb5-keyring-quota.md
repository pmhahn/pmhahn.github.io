---
layout: post
title: "Linux Kernel Keyring Quota exceeded"
date: 2020-01-10 12:00:00  +0100
categories: linux debian filesystem security
excerpt_separator: <!--more-->
---

Nach einer kurzen Pause kam ich heute wieder zurück zu meinem Notebook und konnte mich nicht mehr anmelden. Nach einigem Suchen bin ich in `journalctl -u sssd` über folgende Fehlermeldung gestolpert:

	[sssd[krb5_child[18654]: Disk quota exceeded

<!--more-->

[SSSD](https://docs.pagure.org/SSSD.sssd/) ist der *System Security Services Daemo* und hat zur Aufgabe, die Benutzer-, Gruppen- und Login-Daten zu verwalten.
In UCS verwenden wir noch `libpam-ldap` und `libnss-ldap,` sie allesamt ein funktionierendes Netzwerk voraussetzen und damit nicht offlinefähig sind, was aber für ein Notebook Pflicht ist.

SSSD ist modular aufgebaut und kann neben den Benutzer- und Gruppendaten aus dem LDAP auch mit Kerbenos-Keys umgehen:
Mit der Authentifizierung bekommt man sein *Kerberos Ticket Granting Ticket* (KRBTGT), was als Ausweis für eine erfolgreiche Authentifizierung dienst.
Damit weißt man sich gegenüber dem Kerberos-Server aus, wenn man auf weitere Dienste zugreifen will.
Für jeden Dienst erhält man ein weiteres Ticket, was einen für den Zugriff legitimiert.

Diese Tickets müssen natürlich irgendwo zwischengespeichert werden.
Klassischerweise wurde dafür eine Datei wie `/tmp/krb5cc_$UID` genutzt.
Auf diese muss man gut aufpassen, denn solange die Tickets gültig sind kann jeder mit Zugriff auf diese Datei sich als besagter Benutzer ausgeben und auf die Dienste zugreifen.

Aus Aspekten der Sicherheit ist es verpönt, solche Daten unverschlüsselt abzuspeichern (`/etc/*.secret` lässt grüßen).
Auch gibt es Situationen, wo andere Dienste, die gerne auch mit einer eigenständigen Benutzer-ID laufen, dennoch auf diese Schlüssel zugreifen müssen.
Ein prominentes Beispiel dafür ist das *Network File System* (NFS).
Unter anderem deshalb hat der Linux daher einen eingebauten Keyring, in dem man kryptographisches Material ablegen kann.
Mit `keyctl` kann man darauf zugreifen und SSSD kann das auch nutzen.

Die Daten sind sicher im Kernel verfahrt und werden auch nicht versehentlich durch Swapping auf die Festplatte geschrieben.
Allerdings gibt es da ein Problem:
Der Mechanismus ließe sich als DOS-Attacke mißbrauchen, wenn es keinen Quota-Mechanismus gäbe, der den maximalen Speicherplatz beschränkt.
Und genau der hat zugeschlagen, weil der Kerberos-Keyring im Laufe der Zeit zu groß geworden ist.
Über `/proc/sys/kernel/keys/maxbytes` ist die Größe pro Benutzer auf maximal 20.000 Byte beschränkt:

	# grep $UID /proc/key-users
	 2260:    28 28/28 28/200 18733/20000

(Die Bedeutung der Werte ist genauer in <man:keyrings(7)> beschrieben.)
Beim Versuch mich zu authentifizieren hat der SSSD nun ein neues Ticket bekommen und wollte dieses an meinen Keyring anhängen.
Die ist aber wegen der Quota-Limitierung gescheitert.

Ändern lässt sich das global für alle Benutzer so:

	# sysctl -w kernel/keys/maxbytes=64000
	kernel.keys.maxbytes = 64000

Danach war die Anmeldung wieder möglich.
In der Statistik konnte ich danach aus sehen, dass der Speicherplatz nicht ausgereicht hat:

	# grep $UID /proc/key-users
	 2260:    29 29/29 29/200 20127/64000

Persistent machen kann man diese Anpassung z.B. über eine Datei wie `/etc/sysctl.d/sssd.conf`, wo man diese Änderung bei jedem Start durchführt.
