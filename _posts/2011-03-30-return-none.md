---
title: 'return None'
date: '2011-03-30T13:32:58+02:00'
layout: post
categories: python
---

C-Programmierer kennen es zu genüge, das im Fehlerfall solch passenden Werte wie `NULL` oder `-1` zurückgegeben werden.
Leider ist diese Art der Fehlerbehandlung auch in etlichen Python-Programmen gang und gebe, die meiner bescheidenen Meinung die Fehlersuche und Behandlung erschweren.

Das Problem ist meiner Meinung nach, dass man sich für den Fehlerfall kreative Rückgabewerte überlegen muss, die dann bei falscher oder sogar fehlender Fehlerbehandlung entsprechend kreativ interpretiert werden und dazu führen, dass die seltsamsten Folgefehler auftreten, die dann nur noch schwerer zu finden sind.
Prominentes Beispiel dafür ist sicherlich [Ariane 5 Flug 501](http://de.wikipedia.org/wiki/Ariane_V88).

Ausnahmen (`try-raise-except`) bieten den Vorteil, dass sie einen Traceback enthalten, der dem Entwickler genau zeigt, an welcher Stelle ein Problem erkannt wurde und zwingen den Programmierer zu einer besseren Fehlerbehandlung, weil ansonsten i.d.R.
das Programm beendet wird.
Hier mag man jetzt sicherlich auch über die Vor- und Nachteile eines solchen Vorgehens streiten, aber mir sind [**Fail-Fast**](http://en.wikipedia.org/wiki/Fail-fast)-Systeme (zumindest als Entwickler) deutlich lieber als byzantinisches Verhalten, bei dem dann erst viel später an unvorhergesehenen Stellen die Fehler zuschlagen.

Einen weiteren Vorteil sehe ich darin, dass man sich nur an den Stellen um die Fehlerbehandlung kümmern kann, an denen es **sinnvoll möglich** ist:
Oft tritt ein Fehler in tief verschachtelten Funktionsaufrufen auf und eine Fehlerbehandlung ist nicht auf jeder Ebene möglich.
Ohne Exceptions muss man auf jeder Ebene Fehler explizit abfangen und kreative Rückgabewerte in noch kreativere Rückgabewerte übersetzen.
Mit Ausnahmen kann man sich den Luxus leisten, einen Fehler nicht zu behandeln, weil es an dieser Stelle im Code einfach keine sinnvolle Alternative gibt.

Das ganze funktioniert allerdings nur dann, wenn man Exceptions richtig© und nicht so wie hier verwendet:
Ein `raise Exception()` zwingt einen dazu, diese und dadurch auch **alle anderen** möglichen Ausnahmen durch ein `except Exception` zu fangen.
Deshalb ist es wichtig, Ausnahmen als eine Hierarchie von Ausnahmen zu betrachten und für die eigenen Komponenten **sinnvolle eigene Ausnahmen** zu definieren.
In Python sind dies nur zwei Zeilen:

```python
class ProxyConfigurationError(ConfigurationError):
    pass
```

Dadurch kann man dann an anderer Stelle leicht unterscheiden, ob ich Fehlkonfigurationen bei den Proxy-Einstellungen anders behandeln will, oder ob ich alle Fehlkonfigurationen über einen Kamm scheren will, weil eine Unterscheidung bei der Fehlerbehandlung an dieser Stelle keinen Sinn macht.

Mir ist bekannt, dass es zu diesem Thema durchaus sehr unterschiedliche Meinungen gibt, von daher freue ich mich schon auf eine Erfahrungen mit dem Thema und eure Kommentare dazu.

{% include abbreviations.md %}
