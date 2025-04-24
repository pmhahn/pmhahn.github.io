---
title: 'UNIX 114: ldapsearch'
date: '2020-06-09T10:37:14+02:00'
layout: post
categories: shell
---

`ldapsearch` hat einige nützliche Parameter:

- `-LLL` zum ausblenden der (meist unnötigen) LDIF-Kommentare.
- `-Q` unterdückt die Ausgabe der SASL-Bibliothek (aber auch deren ggf. Nachfrage nach einem Passwort).
- `-o ldif-wrap=no` Um das Umbrechen nach 72 Zeichen zu unterbinden. Alternative: ldapsearch-wrapper.
- `-S cn` Sortierte die Einträge **Client**-seitig nach einem Attribut.
- `-E sss=cn` Sortiert die Einträge **Server**-seitig nach einem Attribut. In OpenLDAP muss dazu `slapo-sssvlv` aktiviert sein.
- `-E mv='(objectClasses=inetOrgPerson)'` Zeigt nur die Attribute an, auf die der zusätzliche "Matched Value"-Filter passt.

Weitere nützliche Anfragen:

## LDAP Basis herausfinden

```bash
ldapsearch -xLLLo ldif-wrap=no -s base -b '' namingContexts
```

## LDAP Schema Information auslesen

```bash
ldapsearch -xLLLo ldif-wrap=no -s base -b cn=Subschema +
```

Sieh dazu auch [UNIX 108: ldapsearch]({% post_url 2018-10-25-unix-108-ldapsearch %})

## Zugriff über den UNIX-Domain-Socket

```bash
ldapsearch -QY EXTERNAL -H ldapi:///
```

Das funktioniert auch für `ldapmodify` und umgeht ähnlich wie `cn=admin` jegliche ACL!

## Auslesen der slapd Konfiguration

```bash
ldapsearch -LLLo ldif-wrap=no -QY EXTERNAL -H ldapi:/// -b cn=config
```

Das funktioniert auch für `ldapmodify` zum Ändern von Einstellungen im laufenden Betrieb. Siehe [UNIX 112: cn=config]({% post_url 2020-04-08-unix-112-cnconfig %}).

## Suchen nach dem `dn`

`dn` ist **kein** Attributname, ein `ldapsearch … dn` ist also strenggenommen falsch. Korrekt wäre ldapsearch … 1.1.

## "Vererbung" aus dem `dn`

LDAP ist hierarchisch, d.h. der Benutzer mit `dn: uid=phahn,ou=Entwicklung,o=Univention,l=Oldenburg,st=NDS,c=de` erbt **indirekt** das Country, den State, die Location/Organisation/-einheit.
Die `objectClass` `organizationalPerson` erlaubt auch das explizite erfassen der Attribute, aber man kann auch auf die Attribute aus dem `dn` filtern:
```bash
ldapsearch … '(&(uid=*)(l:dn:=Oldenburg))'
```
findet alle Benutzer unterhalb eines Containers `Location` "Oldenburg".

Der Versuch, alle realen Benutzer aus unserem LDAP-Verzeichnis zu bekommen, könnte also so aussehen, enthält aber immer noch zu viel Schrutz:

```bash
ldapsearch -LLLo ldif-wrap=no \
 -b cn=users,dc=knut,dc=univention,dc=de \
 '(&(uid=*)(!(cn:dn:=external))(!(cn:dn:=searchuser))(!(cn:dn:=disabled))(!(cn:dn:=otrs))(!(cn:dn:=mail))(!(cn:dn:=ressourcen)))' \
 1.1
```
