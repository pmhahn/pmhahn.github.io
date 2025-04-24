---
layout: post
title: "Getting rid of LDAP from NextCloud"
date: 2025-03-26 07:15:00  +0100
categories: linux
excerpt_separator: <!--more-->
---

Several years ago I did setup OwnCloud.
I managed users and groups in OpenLDAP, as I had another use-cases, where I needed that information for authentication.

Several years later I switched to NextCloud and got rid of that other use-case.
Managing users and groups have become a pain in LDAP:
I have to use CLI tools instead of the GUI.
While I can do it, others don't like it.
So I would like to uninstall OpenLDAP.

Nicols ask the same question and developed a solution: [Import LDAP users & get rid of LDAP](https://help.nextcloud.com/t/import-ldap-users-get-rid-of-ldap/56629/)
It is a bit outdated and contains some errors.
So here is my step-by-step guide.

<!--more-->

```console
$ cd /srv/www/nextcloud
$ su -s /bin/bash www-data
$ ./occ maintenance:mode --on
$ grep --color db config/config.php
$ mysql --user owncloud --password owncloud
```

```sql
> MariaDB [owncloud]> SELECT * FROM oc_ldap_user_mapping;
+----------------------------------------------+-----------------+--------------------------------------+------------------------------------------------------------------+
| ldap_dn                                      | owncloud_name   | directory_uuid                       | ldap_dn_hash                                                     |
+----------------------------------------------+-----------------+--------------------------------------+------------------------------------------------------------------+
> MariaDB [owncloud]> SELECT * FROM oc_users;
+-------+--------------+--------------------------------------------------------------+-----------+
| uid   | displayname  | password                                                     | uid_lower |
+-------+--------------+--------------------------------------------------------------+-----------+
> INSERT INTO oc_users (uid, uid_lower) SELECT owncloud_name, LOWER(owncloud_name) FROM oc_ldap_user_mapping;
> DELETE FROM oc_ldap_user_mapping;

> MariaDB [owncloud]> SELECT * FROM oc_ldap_group_mapping;
+---------------+-------------------------------------------+--------------------------------------+------------------------------------------------------------------+
| owncloud_name | ldap_dn                                   | directory_uuid                       | ldap_dn_hash                                                     |
+---------------+-------------------------------------------+--------------------------------------+------------------------------------------------------------------+
> MariaDB [owncloud]> SELECT * FROM oc_groups;
+-------+-------------+
| gid   | displayname |
+-------+-------------+
> INSERT INTO oc_groups (gid, displayname) SELECT owncloud_name, owncloud_name FROM oc_ldap_group_mapping;
> DELETE FROM oc_ldap_group_mapping;

> MariaDB [owncloud]> SELECT * FROM oc_ldap_group_membership;
+----+--------------+--------------+
| id | groupid      | userid       |
+----+--------------+--------------+
> MariaDB [owncloud]> SELECT * FROM oc_group_user;
+-------+--------+
| gid   | uid    |
+-------+--------+
> INSERT INTO oc_group_user (gid, uid) SELECT groupid, userid FROM oc_ldap_group_membership;
> DELETE FROM oc_ldap_group_membership;

> \q
```

```console
$ ./occ app:disable user_ldap
$ ./occ maintenance:mode --off
```

{% include abbreviations.md %}
