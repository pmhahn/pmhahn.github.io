---
title: 'APT: Install packages from apt/'
date: '2021-12-03T17:15:30+01:00'
layout: post
categories: debian
---

Q: How can I install UCS packages from apt/ (on demand)?

A: Setup `omar` as an additional APT package repository but with a lower priority than the default UCS package repository.

In an [old posting]({% post_url 2011-05-23-pakete-aus-verschiedenen-repositories %}) `origin` was used to reduce the priority of the additional repository.

I have a script `~phahn/bin/VM-SETUP` which I use to setup all my VMs.
Next to setting several UCR variables it also setups `omar/build2/` as an additional Debian package source, but with a low priority to not install packages from there by default but on demand.

## `/etc/apt/sources.list.d/99omar.list`

```
deb [trusted=yes] http://omar.knut.univention.de/build2/ ucs_5.0-0-errata5.0-0/all/
deb [trusted=yes] http://omar.knut.univention.de/build2/ ucs_5.0-0-errata5.0-0/$(ARCH)/
```

Do not put this in `/etc/apt/sources.list` as the files are processed in order and that location is preferred over all in `/etc/apt/sources.list.d/`.
If a Debian package is available from multiple locations it will be downloaded from the first location.
At least make sure to remove this file if you do errata release QA as otherwise you will test downloading from `omar` instead of `updates.software-univention.de`.

## `/etc/apt/preferences.d/99omar.pref`

```
Package: *
Pin: origin omar.knut.univention.de
Pin-Priority: 499
```

The default priority for packages is **500** unless you have configured `APT::Default-Release` in which case that release will have priority **990**.
The given priority of **499** prevents those packages from being installed even though they have a higher version number.

After an `apt-get -qq update` you can check the downloaded `Releases` files, which will include a `Suite`:

```console
# ( cd /var/lib/apt/lists && grep ^Suite: *Release )
omar.knut.univention.de_build2_ucs%5f5.0-0-errata5.0-0_all_Release:Suite: apt
omar.knut.univention.de_build2_ucs%5f5.0-0-errata5.0-0_amd64_Release:Suite: apt
updates.knut.univention.de_dists_errata500_InRelease:Suite: errata500
updates.knut.univention.de_dists_ucs500_InRelease:Suite: ucs500
```

By using the option `-t apt` you implicitly change `APT::Default-Release` to one of these suites and thus change the priority to **990**.

```console
# apt-cache policy univention-directory-listener -t apt
univention-directory-listener:
  Installiert:           14.0.5-6A~5.0.0.202111301701
  Installationskandidat: 14.0.5-6A~5.0.0.202111301701
  Versionstabelle:
     14.0.5-4A~5.0.0.202111151258 990
        990 http://omar.knut.univention.de/build2 ucs_5.0-0-errata5.0-0/amd64/ Packages
 *** 14.0.5-3A~5.0.0.202106081249 500
        500 http://updates.knut.univention.de errata500/main amd64 Packages
        100 /var/lib/dpkg/status
     14.0.5-2A~5.0.0.202104221558 500
        500 http://updates.knut.univention.de ucs500/main amd64 Packages
```

Only then the packages from that repository will be considered.

```bash
apt-get install -t apt univention-directory-listener
apt-get install univention-directory-listener/apt
apt-get upgrade -t apt
```

Please note, that appending `/apt` to the package name will only install that specific package version, which might not work as it requires sister packages from the same repository with a specific version.
In contrast to that `-t apt` will allow this as this changes the priority of the whole repository, which also boosts the priority of those sister packages.

See [`apt_preferences(5)`](man:apt_preferences(5)) for more details.

Be warned that using `APT::Default-Release` is considered problematic and using `preferences` is preferred.
Especially with Debian using just the release name will break installing security updates as they have a different name.
Make sure to use something like `APT::Default-Release "/^bullseye(|-security|-updates)$/";`.

{% include abbreviations.md %}
