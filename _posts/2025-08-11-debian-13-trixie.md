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
Postfix was complaining that its `lmtp` service was no longer able to establish an encrypted connection to `lmtpd` from Cyrus.

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

## `docker.io` and `libvirt`

For some unknown reason `docker.io` and `libvirt` got removed during the upgrade.
Running `apt autopurge` afterwards was a very bad idea as that purged all images, volumes and containers. 🤦

I have to investigate why that happened. 🔍

## PHP-8.4

Debian-13-Trixie has PHP-8.4, while Debian-12-Bookworm had PHP-8.2.
My local NextCloud (and Wordpress) setup was unhappy about that as it needs several `php8.4-…` packages.
Luckily just installing the equivalent of the matching packages did fix this.

## KDE

In the past I did not install `kde-full` as it depends on many optional packages like KMail, KOrganizer, DragonPlayer, and such.
I don't use may of those and thus and thus don't want them to be installed.
During the upgrade `plasmashell` got removed so on the next login I did not get back a working KDE session.
Installing `kde-standard` fixed this.
As it only `Recommends` most other packages, I was able to get rid of those packages I did not want.

And I got Wayland, which has this annoying bug: Konsole no longer stores the open sessions and starts with only one shell in `$HOME`. 🤔

## Out-of-space `/usr`

My desktop system has many packages.
Upgrading all those (KDE-)libraries required too much space on `/usr`.
`dpkg` failed to unpack a package during upgrade.

After some manual `dpkg --configure --pending`, `apt install --fix-broken`, `apt autopurge` and `dpkg -P` I was finally able to continue.
I would have expected for APT to check for enough disk space, but apparently it does not.
So double-check manually before doing an upgrade.

PS: Afterwards `systemd` complains about `use-not-merged`, but that is [normal and expected](https://www.debian.org/releases/trixie/release-notes/issues.html#systemd-message-system-is-tainted-unmerged-bin).

## KeePassXC

I used a self-compiled version of KeePassXC.
Debian now has two packages `keepassxc` and `keepassxc` – the later has support for browser-integration and more.
As some file have been move, the upgrade failed and I had to manually remove by self-compiled version.

## Network

Running the upgrade while being logged into KDE is not a good idea:
During the upgrade NetworkManager got restarted and killed by local network connection.
Afterward even `ping` did no longer work, as I already had the [new version](https://www.debian.org/releases/trixie/release-notes/issues.de.html#ping-no-longer-runs-with-elevated-privileges) but still the old Linux kernel.

Sadly I still need my `r8168-dkms` and `v4l2loopback-dkms` packages.
