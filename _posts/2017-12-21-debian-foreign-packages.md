---
layout: post
title: "Find Debian packages installed from removed source"
date: 2017-12-21 17:06:00  +0100
categories: debian
---

Sometimes you add a temporary APT source to `/etc/apt/sources.list`, install same packages and remove the repository again.
Or you install some `.deb` file directly, which you copied by hand to your environment.

Here is some handy shell command, to find those packages, which are

* installed on your system
* but have no version in any of the currently configured APT repositories:

```bash
dpkg-query -W -f '${Package}\n' |
xargs apt-cache policy |
sed -ne '/^ /{H;$!b};x;/\*\*\* [^\n]*\n[^\n]*\/var\/lib\/dpkg\/status/p'
```

How does it work:

* We use `dpkg-query` to get a list of currently installed packages.
* We pipe that list to `apt-cache`, which prints the list of all known package versions.
* We use a `sed` script to
    * collect all belonging to one package in the *hold* buffer
    * print out that buffer if the current version (marked by `***`) being known in `/var/lib/dpkg/status` only.

*[APT]: Advanced Packaging Tool
