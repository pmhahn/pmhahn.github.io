---
title: 'Debian 13 Trixie released'
date: '2025-08-11T10:31:00+02:00'
layout: post
categories: debian
---

Last Saturday - 2025-08-09 - [Debian 13 "Trixie"](https://www.debian.org/News/2025/20250809) has been released after 2 years of work. 🥳

I just updated my laptop and servers and stumbled upon some issues:

<!--more-->

## `cyrus-imapd`

I'm running my own mail server infrastructure:
I have grown up with _Unix-to-Unix-Copy-Protocol_ (UUCP) and was an admin for _UUCP Freunde Lahn e.V._ for a long time.
I'm still using _Postfix_ with _Cryus IMAPd_ and never switched to _Dovecot_.

After the upgrade I noticed that no mails were delivered:
`mailq` shown a growing list of mails stuck in queue.
Postfix was complainging that its `lmtp` service was no longer able to establish an encrypted connection to `lmtpd` from Cyrus.

For historic reason my setup is using `STARTTLS`, which is now deprecated and has been disabled by default in Cyrus IMAPd.
You have to explicitly re-enable it in your `/etc/imapd.conf` by adding some lines:
```
imap_allowstarttls: yes
lmtp_allowstarttls: yes
```

## `saslauthd`

`saslauthd.service` failed to start as I had to move its UNIX socket to `/var/spool/postfix/var/run/saslauthd/`.
This also moves the location of the PID file to that directory, which then no longer matches the information in `/usr/lib/systemd/system/saslauthd.servie`, which expects the file in `/var/run/saslauthd.pid`.

A fixed this by creating an override with `systemctl edit saslauthd.service`:
```ini
[Service]
PIDFile=/var/spool/postfix/var/run/saslauthd/saslauthd.pid
```

Previously I had a shell-hack in `/etc/default/saslauthd` to replace the old location with a symbolic link to the `chroot`-location.
This no longer works as that file is no sourced by `systemd`, which does not execute that shell code.
Therefore I had to tell Cyrus IMAPd to also use that changed location by putting this into `/etc/imapd.conf`:
```
sasl_saslauthd_path: /var/spool/postfix/var/run/saslauthd/mux
```

PS: On a side node: `/var/run/` is deprecated and should be replaced by just `/run/`; `systemd` already complains about this every time it sees `/var/run/`.

## `docker.io`

For some unknown reason `docker.io` got removed during the upgrade.
Running `apt autopurge` afterwards was a very bad idea as that purged all images, volumes and containers. 🤦

I have to investigate why that happened.

## PHP

Debian-13-Trixie has PHP-8.4, while Debian-12-Bookworm had PHP-8.2.
My local NextCloud setup was unhappy about that as it needs several `php8.4-…` packages.
Luckily just installing the equivalent of the mathcing packages did fix this.

## KDE

In the past I did not install `kde-full` as it depends on many optional packages like KMail.
I don't use may of those and thus and thus don't want them to be installed.
During the upgrade `plasmashell` got removed so on the next login I did not get back a working KDE session.
Installing `kde-standard` fixed this.
As it only `Recommends` most other packages, I was able to get rid of those packages I did not want.

And I got Wayland, which has this annoying bug: Konsole no longer stores the open sessions and starts with only one shell in `$HOME`. 🤔
