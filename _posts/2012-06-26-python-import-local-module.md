---
title: 'Python import local.module'
date: '2012-06-26T09:08:42+02:00'
excerpt: 'Das Geheimnis von package.__path__'
layout: post
categories: python
---

Beim schreiben von Unit-Tests für Python-Module hatte ich bisher immer das Problem, daß das zu testende Modul zum Zeitpunkt, wo das Module gebaut ist, natürlich noch nicht in dieser Version installiert ist; im ungünstigen Fall ist gar eine alte Version der Python-Module installiert, gegen die die Tests dann laufen. Dummerweise liegen viele Pakete innerhalb des `univention`-Pakets, so daß es dann auch gerne Probleme damit gibt, daß entweder das eigene Paket `univention.foo` oder solche globalen Pakete wie `univention.config_registry` nicht gefunden werden.

Die Lösung ist in der [`__path__`-Variable](http://docs.python.org/tutorial/modules.html#packages-in-multiple-directories) versteckt:
```python
import os.path
import univention
univention.__path__.insert(0, os.path.abspath("modules/univention"))
import univention.updater.tools
import univention.config_registry
print(univention.updater.tools.__file__)
# /root/univention-updater/modules/univention/updater/tools.pyc
print(univention.config_registry.__file__)
# /usr/lib/pymodules/python2.6/univention/config_registry.pyc
```

{% include abbreviations.md %}
