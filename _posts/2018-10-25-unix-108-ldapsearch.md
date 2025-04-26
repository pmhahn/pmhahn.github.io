---
title: 'UNIX 108: ldapsearch'
date: '2018-10-25T14:37:27+02:00'
layout: post
categories: shell
tags: ldap
---

Q: Wie war noch mal die Definition von **inetOrgPerson** im LDAP Schema?

A:

```bash
ldapsearch -xLLLo ldif-wrap=no -b cn=Subschema -s base \
 objectClasses -E mv='(objectClasses=inetOrgPerson)'
```

oder die vom Lieblingsgetränk:
```bash
ldapsearch -xLLLo ldif-wrap=no -b cn=Subschema -s base \
 attributeTypes -E mv='(attributeTypes=drink)'
```

Ohne den **Matched Value**-Filter bekommt man ansonsten alle Objektklassen bzw. Attribute und muss dann per `grep` die richtigen Einträge finden.

Das ganze funktiniert auch für mehrere Werte wie folgt:

```bash
ldapsearch -xLLLo ldif-wrap=no -b cn=Subschema -s base \
 attributeTypes -E mv='((attributeTypes=drink)(attributeTypes=carLicense))'
# Kein | hier ---------^^
```

Zum Nachlesen [RFC 3876](https://datatracker.ietf.org/doc/html/rfc3876 "Returning Matched Values with the Lightweight Directory Access Protocol version 3 (LDAPv3)").

{% include abbreviations.md %}
