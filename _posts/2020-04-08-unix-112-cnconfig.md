---
title: 'UNIX 112: cn=config'
date: '2020-04-08T17:42:26+02:00'
layout: post
categories: UCS
tags: ldap
---

Q: Der OpenLDAP-Server liefert mir zu wenig Daten und ich kann oder will ihn aber im moment nicht neu starten.
Was kann ich tun?

A: [Bug #16639](https://forge.univention.org/bugzilla/show_bug.cgi?id=16639) macht’s möglich:
```bash
printf 'dn: cn=config\nchangetype: modify\nreplace: olcLogLevel\nolcLogLevel: %d\n\n' 256 |
 ldapmodify -xH ldapi:///
ldapsearch -xLLLo ldif-wrap=no -H ldapi:/// -s base -b 'cn=config' olcLogLevel
```

Siehe auch [man:slapd.conf(5)](https://www.openldap.org/doc/admin24/slapdconfig.html#loglevel%20%3Clevel%3E), da man den numerischen Wert angeben muss.

Man beacht übrigens dass `none` nicht gleich `0` ist:
letzteres loggt gar nichts mehr!
Arvid hat mir mal gegenüber gesagt, dass alleine minimales Logging OpenLDAP (stark) verlangsamt, aber das habe ich jetzt selber nicht recherchiert.

{% include abbreviations.md %}
