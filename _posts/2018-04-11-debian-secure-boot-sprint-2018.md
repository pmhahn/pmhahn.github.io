---
title: 'Debian Secure-Boot Sprint 2018'
date: '2018-04-11T11:14:47+02:00'
layout: post
categories: debian
---

Letzte(s) Woche(nende) war ich in Fulda auf dem [Debian Secure-Boot Sprint](https://wiki.debian.org/Sprints/2018/SecureBootSprint).
Ziel war es bei [Debian](https://www.debian.org/) für die nötige Infrastruktur zu sorgen, damit EFI-Binaries und Linux-Kernel-Module dort automatisch signiert werden können.
Die Herausforderung für Debian besteht darin, das es dort viele Entwickler gibt, die Zugriff auf diese Infrastruktur brauchen:

- das Team der GRUB-Maintainer
- das Team der Linux-Kernel-Maintainer
- der SHIM-Maintainer
- der Maintainer vom fwupdate-Paket
- die Mitglieder des Security-Teams

Diese sind weltweit verstreut und brauchen alle Zugriff auf den den einen Schlüssel, der im SHIM von Debian verankert ist.
Microsoft fordert aber in seiner [Richtlinie](https://blogs.msdn.microsoft.com/windows_hardware_certification/2013/12/03/microsoft-uefi-ca-signing-policy-updates/), das dieser private Schlüssel in einem Hardware-Token gespeichert ist, was Debian vor das Problem stellt, eine zentrale Infrastruktur für das Signieren zu haben.

Ein [erster Ansatz](https://wiki.debian.org/SecureBoot#Proposed_signing_architecture) war, zu signierende Pakete manuell an einen Dienst zu senden, der dann die notwendigen Signaturen generiert und an den Entwickler zurück sendet.
Dieser hätte dann mehr oder minder jedesmal von Hand ein weiteres Paket mit diesen Signaturen hochladen und bauen müssen, was als zu aufwändig angesehen wurde.
(Es entspricht aber dem Prozess, was wir bisher intern hier bei Univention mit unseren Paketen gemacht haben.
Das es Aufwand macht haben wir auch gelernt, weshalb wir ein Interesse daran haben, dass es zukünftig Debian für uns macht.
Deswegen war ich auch für Univention dabei.)

[Ansatz zwei](https://wiki.debian.org/SecureBoot#Second_option:_use_buildd_.2B-_debhelper_instead_of_dak) war, einen Debhelper hinzuzufügen, der während dem Prozess des Paketbaus den Signaturdienst kontaktiert und on-the-fly die Signaturen generiert und hinzufügt.
Das wurde aber verworfen, weil die Rechner, die für Debian Pakete bauen (buildd) weltweit verteilt sind und immer die Gefahr besteht, das diese kompromittiert werden.

Der nun [gewählte Ansatz](https://etherpad.wikimedia.org/p/debian-secure-boot-2018) sieht so aus, das die betroffenen Pakete ein sog. "signing-template" Binär-Paket während dem normalen Paket-Build-Prozess erstellen.
Dieses enthält eine Liste der zu signierenden Pakete bzw. deren Dateien (`files.json`) und die Vorlage für ein neues Quellpaket.
Wird ein solches Paket durch den Maintainer (oder einen buildd) hochgeladen, triggert das in DAK ([Debian Archive Kit](https://wiki.debian.org/DebianDak), das Äquivalent zu `repo-ng`) das Signieren:
Der [code-signing-Service](https://salsa.debian.org/ftp-team/code-signing) wird getriggert und lädt sich dieses Template-Paket herunter, extrahiert es lokal und analysiert die `files.json`-Datei.
Anschließend lädt es die darin benannten Pakete herunter, extrahiert diese ebenfalls lokal und generiert die angeforderten Signaturen.
Diese werden dann der Vorlage unterhalb von `debian/signatures/` hinzugefügt und als neues Quellpaket zu Debian hochgeladen.
Dieses durchläuft dann erneut den normalen Debian-Build-Prozess (DAK, wanna-build, buildd), der i.d.R. die so generierten Signaturen an die ursprünglichen Binaries anhängt und an der richtigen Stelle ablegt.
(GRUB verwendet z.B. für die signierten Dateien einen anderen Pfad, während Linux-Kernel-Module an der selben Stelle zu liegen haben).

Den Prozess gibt es doppelt:
Einmal für reguläre Updates und ein zweites Mal für Security-Updates:
Dort kann es nämlich vorkommen, das es eine Embargo-Periode gibt, in der nichts über eine Sicherheitslücke öffentlich werden darf.
Trotzdem sollen natürlich vorab schon die Pakete gebaut werden, die ggf. auch Signaturen erfordern.
Das alles passiert in einem gesonderten Bereich, der erst öffentlich sichtbar wird, wenn das Embargo abgelaufen ist.

Um die Sicherheit zu erhöhen gibt es noch 2 Features:

1. damit Pakete überhaupt den Signing-Service nutzen können, müssen die Template-Pakete auf einer Whitelist stehen.
   Ansonsten könnte jeder Debian-Entwickler sich Binaries signieren lassen, die sich an in Secure-Boot-Umgebungen booten ließen.
2. Der Signing-Service protokolliert jede Signatur, so dass Unregelmäßigkeiten erkannt werden können.
   Im schlimmsten Fall müsste Debian seinen SHIM widerrufen wenn bekannt wird, das Signaturen unberechtigt erzeugt wurden.

Aktueller Stand ist, das wir (Debian) nun alle notwendige Infrastruktur haben (DAK, signing-code).
Auch wurde schon alle wichtigen Pakete (Linux Kernel, Shim, GRUB, fwupdate) angepasst, um die signing-templates zu erzeugen.
Was noch fehlt ist der vollautomatische Betrieb und das hochladen der angepassten Pakete.

{% include abbreviations.md %}
