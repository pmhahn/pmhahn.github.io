---
title: 'Suche mit aptitude'
date: '2011-04-05T16:40:05+02:00'
layout: post
categories: linux debian
---

Sönke suchte eine einfache Möglichkeit, alle veralteten Kernel von einem System zu löschen.
Das sollte folgender Einzeiler erledigen:

```bash
aptitude remove '?and(?installed,
?and(?name(linux-image-*),
?not(?reverse-Depends(?and(?name(univention-kernel-image-*),
?version(TARGET))))))'
```

*Aptitude* unterstützt eine Form der Prädikatenlogik, mit der sich [komplexe Suchanfragen](http://algebraicthunk.net/~dburrows/projects/aptitude/doc/en/ch02s03s05.html "Aptitude Search Term Reference") formulieren lassen.
Hier wird diese dazu genutzt, „alle installierten Linux-Kernel-Pakete zu löschen, auf die kein aktuelles Univention-Kernel-Paket mehr verweist.“

Es muß aber nicht immer so kompliziert sein, auch für einfachen Suchanfragen bietet *Aptitude* einige nette Möglichkeiten:
Mit `~d` kann man auch innerhalb von Aptitude aus die Beschreibungen (ähnlich zu `apt-cache search`) durchsuchen, oder mit `~i` nur nach installierte Paketnamen.
Mehrere durch Leerzeichen getrennte Suchbegriffe werden durch *Und* verknüpft.

{% include abbreviations.md %}
