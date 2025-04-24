---
title: 'CD-ROM-Image neu laden nach ISO-Neubau'
date: '2012-01-04T14:07:03+01:00'
layout: post
categories: virt
---

Wenn KVM-Instanzen das latest-ISO-Image einbinden kommt es immer wieder vor, daß nach dem Neubau der Image-Datei die VM Lesefehler meldet:

> Buffer I/O error on device sr0, logical block xxxx
> attempt to access beyond end of devie

Das liegt daran, das beim Starten der VM die ursprüngliche ISO-Datei geöffnet wurde und die Meta-Daten darauf beim Mounten im Gast-Betriebssystem eingelesen wurden. Nach dem Neubau passen dann diese Daten nicht mehr zum dann aktuellen Inhalt der ISO-Datei.
Entweder muß man über UVMM das Medium im CD-ROM-Laufwerk wechseln (aus- und wiedereinhängen), oder man wirft das Image eben per Kommandozeile aus und sofort wieder ein:
```bash
eject /dev/sr0 && eject -t /dev/sr0
```

{% include abbreviations.md %}
