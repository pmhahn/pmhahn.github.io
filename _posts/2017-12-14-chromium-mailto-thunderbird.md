---
layout: post
title: "Chromium and mailto: URLS"
date: 2017-12-14 11:49:00  +0100
categories: linux
---

[RFC 2368](https://tools.ietf.org/html/rfc2368) specified the `mailto:` syntax.
It was superseded by [RFC 6068](https://tools.ietf.org/html/rfc6068), which added UTF-8 support.
It can be used to launch your email client by clicking on some URL.

Chromium uses `xdg` since [Issue 61942](https://bugs.chromium.org/p/chromium/issues/detail?id=61942).
But getting it to work correctly seems to be hard.

Overview
========
There is a specification for opening URLs and files.
It is called *X Desktop Group* (XDG), now renamed to [freedesktop.org](http://freedesktop.org).
You can use `xdg-open` to open a file.
It will pick the right application automatically.

There also is `xdg-email` to compose new emails.
`xdg-email` only understands a limited subset of options:
* `to`
* `cc` (Carbone Copy)
* `bcc` (Blind Carbone Copy)
* `subject`
* `body`
* `attach`

You can launch an email client yourself like this:

```bash
xdg-open 'mailto:pmhahn@pmhahn.de?subject=Subject&body=Body'
xdg-email 'mailto:pmhahn@pmhahn.de?subject=Subject&body=Body'
xdg-email --to pmhahn@pmhahn.de --subject Subject --body Body
```

Issues
------
`xdg-email` supports two syntaxes:
* the split out variant using multiple arguments
* or by either passing a `mailto:` URL, which is passed unmodified (with **one** exception)

As `xdg-email` only understands the options listed above, other options ones like `In-Reply-To` **might** get ignored.
But this depends on your environment.

Custom script
=============
You can create a custom script named `xdg-email-hook.sh`.
It has the highest priority.
This is quiet useful to disable any further URL and command line processing:
Newer versions of Thunderbird seem to support URL-handling themselves:

```bash
thunderbird -h
# Usage: /usr/lib/thunderbird/thunderbird [ options ... ] [URL]
```

You can use this to pass through other mail headers like `In-Reply-To`.
Create a file `xdg-email-hook.sh` in a directory, which is searched by `PATH`.

* Make it executable
* Use the following content:

```bash
#!/bin/sh
exec /usr/bin/thunderbird "$@"
```

MAILER
======
You can user the environment variable `MAILER` to select your preferred mail client.
It has the second highest priority.
You can configure multiple clients, separated by `:`.
They are tried in that order until one succeeds.

Desktop Environments
====================
Otherwise `xdg-email` will try to determine your Desktop Environments:

* it evaluates the environment variable `XDG_CURRENT_DESKTOP`.
* it uses the environment variables `KDE_FULL_SESSION`, `GNOME_DESKTOP_SESSION_ID`, `MATE_DESKTOP_SESSION_ID`, `DESKTOP` to detect some common environments.
* it uses `dbus` to query the current session manager

GNOME
-----
It will query `gconftool-2 /desktop/gnome/url-handlers/mailto/command`.
Or use `xdg-mime x-scheme-handler/mailto`.
Or use `gvfs-open` or `gnome-open`.

You can configure *Thunderbird* as your default handler for email like this:

```bash
xdg-mime default /usr/share/applications/thunderbird.desktop x-scheme-handler/mailto
```

XFCE
----
It will use `exo-open` to launch your mail client.

KDE
---
With KDE you have one more indirection:
You need to get the name of your profile.
Then you need to get the currently configured mail client:

```bash
kreadconfig5 --file emaildefaults --group Defaults --key Profile
# Standard
kreadconfig5 --file emaildefaults --group PROFILE_Standard --key EmailClient
# thunderbird %u
```

Further reading
===============
[Arch Linux](https://bbs.archlinux.org/viewtopic.php?id=154031) has a good article about this issue, too.
It was the one with prompted me to write this blog.
