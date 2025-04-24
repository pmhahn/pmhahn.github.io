---
title: 'TCP Keep Alive'
date: '2011-04-19T18:03:12+02:00'
excerpt: 'TCP Keepalive ist standardmäßig nicht aktiv.'
layout: post
categories: linux
---

Privat hatte ich neulich auf einem Server das Problem, daß dort massenhaft `imapd`-Prozesse liefen, die sich selbst nach Tagen nicht beendet haben, wie ein `ps u` mit gezeigt hat.

Grund war, das es sich um eine alte Version des Cyrus-Imap-Servers handelte, der auf das Eintreffen weiterer Imap-Kommandos über eine gesicherte SSL-Verbindung wartete.
Ein `strace -p` hat gezeigt, das dieser brav in einem `read(0)` blockiert war während der Partner dieser TCP-Verbindung schon längst Opfer einer DSL-Zwangstrennung geworden war, die dafür gesorgt hatte, das diese Verbindung nicht ordentlich beendet wurde.

Nun mag sich der ein oder andere Fragen, warum diese Verbindung nicht durch irgendein Timeout beendet wurde?
Nun, die meisten TCP-Timeouts beziehen sich auf die Phase des Verbindungsaufbaus, nicht aber auf den regulären Betrieb!
Da der Server dem Client in diesem Fall nichts mitzuteilen hat, wird nie ein Paket vom Server zum Client gesendet, anhand dessen der Server erkennen könnte, das der Client nicht länger erreichbar ist.
So wartet der Server bis in alle Ewigkeit auf das nächste Paket vom Client.
Eigentlich hat der Imapd-Server extra einen Parameter, über den eine Verbindung per Timeout beendet werden kann, aber diese scheint wohl nicht im Zusammenhang mit SSL zu funktionieren.

Genau deswegen gibt es die TCP-Socket-Option `TCP_KEEPALIVE`:
Dadurch werden regelmäßig leere TCP-Pakete verschickt, um tote Partner zu entdecken.
Da TCP datenstromorientiert arbeitet, werden diese leeren Pakete dort einfach herausgefiltert und werden so für die Anwendung selbst gar nicht sichtbar.

Das Problem dabei ist nur, das diese Funktion explizit aktiviert werden muß:
Vergisst man das, dann erkennen ggf.
langlaufende Prozesse nicht das Verschwinden ihrer Kommunikationspartner und blockieren so ggf. wertvolle Ressourcen.

Von daher bitte immer daran denken, sich darüber bei eigenen Diensten Gedanken zu machen.
Wie das mit Python geht, ist u.a. in diesem [Mailinglisten-Posting](http://mail.python.org/pipermail/python-dev/2010-April/099235.html) beschrieben.

{% include abbreviations.md %}
