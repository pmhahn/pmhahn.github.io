---
title: 'Debian 102: maintainer scripts'
date: '2021-12-14T09:50:44+01:00'
layout: post
categories: debian
---

During Debian package installation, upgrade, downgrade and remove the so called [Debian Package maintainer scripts]({% post_url 2020-06-11-dpkg-compareversions %}) are called before and after certain actions:

- `preinst`
  - before the package is unpacked
  - prepare the environment for unpacking
- `postinst`
  - after the package is unpacked
  - configure the package to make it fully functional
- `prerm`
  - before the package is removed
  - prepare the package for removal or upgrade
- `postrm`
  - after the package is removed
  - cleanup after removal or upgrade

That’s simplified as the scripts are called with additional parameters. These extra arguments allow you to distinguish new installations from upgrades and are also used to handle error cases.

## New installation

```console
# apt-get install $pkg
preinst install ""
postinst configure ""
```

## Upgrade

```
old.prerm upgrade "$new_version"
new.preinst upgrade "$old_version"
old.postrm upgrade "$new_version"
new.postinst configure "$old_version"
```

## Remove and purge

```console
# apt-get remove $pkg
prerm remove
postrm remove
# apt-get purge $pkg
postrm purge
```

## Remove and re-install

Please note that when you only remove a package and later on re-install it, the old version will still be passed to the maintainer script:

```console
# apt-get remove $pkg
prerm remove
postrm remove
# apt-get install $pkg
preinst install "$old_version"
postinst configure "$old_version"
```

Compare this to **purge**, where all its configuration files and state are removed:

```console
# apt-get remove $pkg
prerm remove
postrm remove
# apt-get purge $pkg
postrm purge
# apt-get install $pkg
preinst install ""
postinst configure ""
```

## Upgrade vs. Removal

Please make sure to handle those two cases differently in you `prerm` script:

```bash
#!/bin/sh
set -e
#DEBHELPER#
a2dissite univention-portal.conf
```

This will disable the Apache site also on **every** upgrade! While hopefully it will be re-enabled by the corresponding `postint` this might still lead to service interruption: If any other package restarts the Apache web server in between the Portal will be temporarily unavailable.

So please do it like this:

```bash
#!/bin/sh
set -e
#DEBHELPER#
case "$1" in
  remove)
    a2dissite univention-portal.conf
    ;;
esac
```

`postinst` is less critical when you follow the style to make your scripts [idempotent](https://www.debian.org/doc/debian-policy/ch-maintainerscripts.html#maintainer-scripts-idempotency): Write them so that they achieve the desired **state** (<q>make sure the portal site is enabled</q>) instead of performing an action (<q>add 5 every time the script is executed</q>). Basically make sure your service is configured and running each time `postinst` is called. Most often you can just ignore the parameter there: Only very seldom there is the need to distinguish failed upgrades from new installations, in almost all cases the service should be just running (again).

But `prerm` is a different beast and you should spend some time on thinking about the difference between *removal* and *upgrade*: For a long time it was Debians policy to stop all services before an upgrade and only restart them afterwards. For some services this is still needed as program files cannot be upgraded if a process executing them is still running (a binary executable file is write-protected while a process is executing it). This has lead to long down times especially during upgrades. The rule is relaxed by know and many [debhelper](https://manpages.debian.org/unstable/debhelper/dh_systemd_start.1.de.html) scripts by know support the option `--restart-after-upgrade` respectively `--no-stop-on-upgrade` which delays the required restart until the `postinst` phase.
So please take some time and fix you `postinst` script to stop doing unnecessary work.
