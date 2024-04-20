---
layout: post
title: "A look at OpenLDAP LMDB"
date: 2024-04-15 09:36:00  +2100
categories: linux
excerpt_separator: <!--more-->
---

[OpenLDAP](https://www.openldap.org/) has many backend.
One is [LMDB](https://www.symas.com/lmdb), the (Lightning) Memory-Mapped Database (LMDB).
Read [Howard Chus presentation](http://www.lmdb.tech/media/20130406-LOADays-LMDB.pdf) for technical details.

<!--more-->

# Search
## Filters
* Presence (`pres`): `(objectClass=*)`
* Equality (`eq`): `(objectClass=dNSRecord)`
* Greater-or-equal: `(modifyTimestamp>=20230102030405Z)`
* Less-or-equal: `(modifyTimestamp<=20230102030405Z)`
* Substring (`sub`): `(description=*Developer*)`
  * `subinitial`: `(description=prefix*)`
  * `subany`: `(description=*infix*)`
  * `subfinal`: `(description=*suffix)`
* Approximate (`approx`): `(gn~=Fillip)` using [metaphone](https://en.wikipedia.org/wiki/Metaphone) and [soundex](https://en.wikipedia.org/wiki/Soundex) with OpenLDAP
* NOT filter: `(!(description=Test))`
* OR filter: `(|(aRecord=1.2.3.4)(aAAARecord=1:2:3:4:5:6:7:8))`
* AND filter: `(&(gn=Philipp)(sn=Hahn))`
* Extensible match:
  * DN matching, e.g. `(cn:dn:=test)`
  * Custom matching rule `(givenName:caseExactMatch:=John)`
  * Any matching value `(:caseExactMatch:=John)`

## How it works
* For each part of the filter
  * if there is no index, walk all entries and check the node for matching
  * if there is an index, retrieve the set of matching nodes from the index and modify the bitmap accoringly.
    See [servers/slapd/back-mdb/filterindex.c](https://git.openldap.org/openldap/openldap/-/blob/master/servers/slapd/back-mdb/filterindex.c?ref_type=heads).

# Index
## Types
* `notags`: flag to disable using index for tags, e.g. FIXME
* `nosubtypes`: flag to disable using index for sub-types, e.g. FIXME
* `substr` (deprecated)
* `nolang` (deprecated)

## Configuration
Can be configured with attribute [oldDbIndex](https://www.openldap.org/doc/admin24/slapdconf2.html) in `olcDatabase={…}mdb,cn=config`:

```console
# ldapsearch -QLLLY EXTERNAL -H ldapi:/// -s base -b 'olcDatabase={1}mdb,cn=config' olcDbIndex
dn: olcDatabase={1}mdb,cn=config
olcDbIndex: default sub
olcDbIndex: objectClass pres,eq
```

New attributes can be indexed in background:
```sh
# ldapmodify -QY EXTERNAL -H ldapi:/// <<__LDIF__
dn: olcDatabase={1}mdb,cn=config
add: olcDbIndex
olcDbIndex: sOARecord pres

__LDIF__
```

# LMDB
Instead of mapping the full DN to each entry, LMDB uses a hierarchical structure.
This allows traversal to sibling, child and parent nodes.
This is also required as keys are limited to 511 bytes in LMDB.

## Sub-databases
```console
$ mdb_stat -a /var/lib/univention-ldap/ldap/
Status of ad2i  # Attribute Description → ID
Status of dn2i  # Distinguished Name → ID
Status of id2e  # ID → Entry
Status of objectClass  # Index for attribute
```

* `ad2i` enumerates all attribute definitions.
  Maps a incremental 4 byte integer to the name of the attribute and tag.
* `dn2i` encodes the tree as a adjacency list?
* `id2e` maps the incremental 8 byte integer to the entry.
  Attributes are referenced by index from `ad2i` instead of repeating the name?
* For each indexed attribute there exists (at least?) one additional sub-database.
  It maps the index key to the entry ID (from `id2e).

## Encoding a tree

<!-- ~/REPOS/ucs/management/univention-directory-listener/src/README.md -->

Given a DN, return the matching entry from the tree via [servers/slapd/back-mdb/dn2entry.c](https://git.openldap.org/openldap/openldap/-/blob/master/servers/slapd/back-mdb/dn2entry.c?ref_type=heads).

First map DN to ID via [servers/slapd/back-mdb/dn2id.c](https://git.openldap.org/openldap/openldap/-/blob/master/servers/slapd/back-mdb/dn2id.c?ref_type=heads) and then from ID to entry via [servers/slapd/back-mdb/id2entry.c](https://git.openldap.org/openldap/openldap/-/blob/master/servers/slapd/back-mdb/id2entry.c?ref_type=heads).

```
0"" --> 1"o=A"
    --> 2"o=B" --> 3"ou=C,o=B" --> 4"cn=D,ou=C,o=B"
```

It uses multiple sorted key-value-stored to map the DN to the sequence of records.
The database contains 2 sub-tables to store the tree hierarchically:

### dn2id

```console
$ mdb_dump -p /var/lib/univention-ldap/ldap/ -s dn2i
```

The `dn2id` sub-database is used to store the tree with it nodes and edges between them.
Each node gets a unique ID, for which the database contains *multiple* entries of the following structure:

```c
struct subDN {
	unsigned long id;
	enum { NODE=0, LINK=1 } type;
	char data[0];
};
```

That structure is used to encode two kind of information in the same tree:

1. Links from parent to child
2. Node with link to parent

#### Nodes
Each LDAP entry is mapped to a unique ID which is stored in this sub-database.
This is necessary because LMDB limits the key size to 511 bytes by default.
The mapping structure is organized like a tree of RDNs, which improves lookup performance compared to a plain list of DNs.
The actual attributes and their values are stored in the [id2entry](#id2entry) sub-database.

`key=0` is the root of the LDAP tree, e.g. "".

* 0"": LDAP root
* 1"o=A"
* 2"o=B"
* 3"ou=C,o=B"
* 4"cn=D,ou=C,o=B"

#### NODE
A node is represented as `type=NODE`.
For any *non-root-node* (1-5) it includes reference to its parent node as `id`.
`data` contains the full DN.

* 0"" --> 1"o=A": 1 → (0, NODE, "")
* 0"" --> 2"o=B": 2 → (0, NODE, "")
* 2"o=B" --> 3"ou=C,o=B": 3 → (2, NODE, "o=B")
* 3"ou=C,o=B" --> 4"cn=D,ou=C,o=B": 4 → (3, NODE, "ou=C,o=B")

#### LINK
For any *non-leaf-node* (0,2,3) a value with `type=LINK` will reference the direct child node `id`.
`data` contains the relative DN of the child.

* 0"" --> 1"o=A": 0 → (1, LINK, "o=A")
* 0"" --> 2"o=B": 0 → (2, LINK, "o=B")
* 2"o=B" --> 3"ou=C,o=B": 2 → (3, LINK, "ou=C")

### id2entry
Each LDAP entry has a unique ID.
The ID is allocated on insert of a new DN into [dn2id](#dn2id).
The key is that ID and the value is the data - in our case the serialized LDAP entry.

```console
$ mdb_dump -p /var/lib/univention-ldap/ldap/ -s id2e
```

## Lookup

A DN lookup starts with the right-most (base) RDN:

1. The search starts at the root with `key=0`.

2. For the base RDN a key-value-pair mapping `key=0` to `value=(id, LINK, rdn)` exists, which is returned due to the clever use of [`mdb_dupsort()`](http://www.lmdb.tech/doc/group__mdb.html#gacef4ec3dab0bbd9bc978b73c19c879ae):
   It allows the *same key* to match to *multiple values*.
   Values are sorted, e.g. `id` before `type`.
   This will return the best match, where `type=LINK` and `rdn` matches the searched RDN.

3. The returned `id` matches the next level of the tree.

4. There the search continues with the next RDN.

# Miscellaneous
- How are multi-RDNs handled?

# Further reading
- [Symas LMDB Tech Info](https://www.symas.com/symas-lmdb-tech-info)
