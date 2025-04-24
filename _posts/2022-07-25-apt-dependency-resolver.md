---
layout: post
title: "Debian APT dependency resolver"
date: 2022-07-25 12:53:00  +0200
categories: debian
excerpt_separator: <!--more-->
---

Debian's package manager APT is famous for its inter-package dependency resolving mechanism:
Long before `rpm` based distributions learned how to install dependant packages automatically Debian did do this for many years.
You simply can install a high-level package using `apt-get install $pkg`, which  will then automatically resolve all dependencies:
Dependant packages are downloaded and install along automatically.

This works very well when you just use packages from a single *consistent* source like the *stable* Debian repository.
It mostly also works with multiple repositories, but from time to time the resolver does *strange* things.

Here at Univention GmbH we build our own packages.
Therefore it is essential for our customers that we get the dependencies right.
Today we had the strange behavior, where one of our packages could not be upgraded:
APT decided to refuse the package from getting installed.
Simplify specifying one additional dependency on the command line made it work.

So what happened and why did APT refuse the initial command?

# Resolver

[Dependency resolution](https://github.com/Debian/apt#dependency-resolution) has some very useful information on how the resolver works:
> APT works in its internal resolver in two stages:
> First all packages are visited and marked for installation, keep back or removal. Option `Debug::pkgDepCache::Marker` shows this.
> This also decides which packages are to be installed to satisfy dependencies, which can be seen by `Debug::pkgDepCache::AutoInstall`.
> After this is done, we might be in a situation in which two packages want to be installed, but only one of them can be.
> It is the job of the `pkgProblemResolver` to decide which of two packages 'wins' and can therefore decide what has to happen.
> You can see the contenders as well as their fight and the resulting resolution with `Debug::pkgProblemResolver`.

<!--more-->

There are some options, which can be set to have a look at the internal working of the APT conflict resolver:

```bash
LC_ALL=C apt-get install \
  -t apt \
  -s \
  -o Debug::pkgDepCache::Marker=yes \
  -o Debug::pkgDepCache::AutoInstall=yes \
  -o Debug::pkgProblemResolver=yes \
  -o Debug::pkgProblemResolver::ShowScores=yes \
  univention-s4-connector 2>&1 | less
```

- `-t apt` boosts all packages from my repository named `Suite: apt` in its `Release` file.
- `-s` does a *simulated* install: it only shows what would be done but does not modify the actual state of the system.
- `-o Debug::pkgDepCache::Marker=yes` shows the initial package marking for installation, keep or removal.
- `-o Debug::pkgDepCache::AutoInstall=yes` shows installation of additional dependencies.
- `-o Debug::pkgProblemResolver=yes` enables debug output.
- `-o Debug::pkgProblemResolver::ShowScores=yes` shows the calculated *importance* score of all packages.
- `univention-s4-connector` is the package to install.

# Explanation

```
Reading package lists...
Building dependency tree...
Reading state information...
```
Some progress information.

## Mark phase
```
  MarkInstall univention-s4-connector:amd64 < none -> 14.0.10-2A~5.0.0.202207141231 @un puN Ib > FU=1
  Installing attr as Depends of univention-s4-connector
    MarkInstall attr:amd64 < none -> 1:2.4.48-4 @un uN > FU=0
  Installing python3-univention-connector-s4 as Depends of univention-s4-connector
    MarkInstall python3-univention-connector-s4:amd64 < none -> 14.0.10-2A~5.0.0.202207141231 @un uN > FU=0
  Installing univention-samba4 as Depends of univention-s4-connector
    MarkInstall univention-samba4:amd64 < none -> 9.0.8-2A~5.0.0.202207141233 @un uN Ib > FU=0
    Installing ldb-tools as Depends of univention-samba4
      MarkInstall ldb-tools:amd64 < none -> 2:2.5.2-1A~5.0.0.202207191717 @un uN > FU=0
    Installing libunivention-ldb-modules as Depends of univention-samba4
      MarkInstall libunivention-ldb-modules:amd64 < none -> 8.0.0-7A~5.0.0.202207191820 @un uN > FU=0
    Installing samba as Depends of univention-samba4
      MarkInstall samba:amd64 < none -> 2:4.16.2-1A~5.0.0.202207191731 @un uN Ib > FU=0
      Installing samba-common as Depends of samba
        MarkInstall samba-common:amd64 < 2:4.16.2-1A~5.0.0.202206271026 -> 2:4.16.2-1A~5.0.0.202207191731 @ii umU > FU=0
      Installing samba-common-bin as Depends of samba
        MarkInstall samba-common-bin:amd64 < 2:4.16.2-1A~5.0.0.202206271026 -> 2:4.16.2-1A~5.0.0.202207191731 @ii umU Ib > FU=0
        Installing samba-libs as Depends of samba-common-bin
          MarkInstall samba-libs:amd64 < 2:4.16.2-1A~5.0.0.202206271026 -> 2:4.16.2-1A~5.0.0.202207191731 @ii umU > FU=0
      Installing tdb-tools as Depends of samba
        MarkInstall tdb-tools:amd64 < none -> 1.4.7-1A~5.0.0.202206171840 @un uN > FU=0
      Installing python3-markdown as Recommends of samba
        MarkInstall python3-markdown:amd64 < none -> 3.0.1-3 @un uN > FU=0
      Installing samba-vfs-modules as Recommends of samba
        MarkInstall samba-vfs-modules:amd64 < none -> 2:4.16.2-1A~5.0.0.202207191731 @un uN IPb > FU=0
        Installing libcephfs2 as Recommends of samba-vfs-modules
          MarkInstall libcephfs2:amd64 < none -> 12.2.11+dfsg1-2.1+b1 @un uN Ib > FU=0
          Installing libboost-thread1.67.0 as Depends of libcephfs2
            MarkInstall libboost-thread1.67.0:amd64 < none -> 1.67.0-13+deb10u1 @un uN Ib > FU=0
            Installing libboost-atomic1.67.0 as Depends of libboost-thread1.67.0
              MarkInstall libboost-atomic1.67.0:amd64 < none -> 1.67.0-13+deb10u1 @un uN > FU=0
          Installing librados2 as Depends of libcephfs2
            MarkInstall librados2:amd64 < none -> 12.2.11+dfsg1-2.1+b1 @un uN Ib > FU=0
            Installing libboost-regex1.67.0 as Depends of librados2
              MarkInstall libboost-regex1.67.0:amd64 < none -> 1.67.0-13+deb10u1 @un uN > FU=0
            Installing libibverbs1 as Depends of librados2
              MarkInstall libibverbs1:amd64 < none -> 22.1-1 @un uN Ib > FU=0
              Installing libnl-route-3-200 as Depends of libibverbs1
                MarkInstall libnl-route-3-200:amd64 < none -> 3.4.0-1 @un uN > FU=0
              Installing ibverbs-providers as Recommends of libibverbs1
                MarkInstall ibverbs-providers:amd64 < none -> 22.1-1 @un uN > FU=0
            Installing libnspr4 as Depends of librados2
              MarkInstall libnspr4:amd64 < none -> 2:4.20-1 @un uN > FU=0
            Installing libnss3 as Depends of librados2
              MarkInstall libnss3:amd64 < none -> 2:3.42.1-1+deb10u5 @un uN > FU=0
        Installing libgfapi0 as Recommends of samba-vfs-modules
          MarkInstall libgfapi0:amd64 < none -> 5.5-3 @un uN Ib > FU=0
          Installing libgfrpc0 as Depends of libgfapi0
            MarkInstall libgfrpc0:amd64 < none -> 5.5-3 @un uN Ib > FU=0
            Installing libgfxdr0 as Depends of libgfrpc0
              MarkInstall libgfxdr0:amd64 < none -> 5.5-3 @un uN Ib > FU=0
              Installing libglusterfs0 as Depends of libgfxdr0
                MarkInstall libglusterfs0:amd64 < none -> 5.5-3 @un uN > FU=0
    Installing univention-samba-local-config as Depends of univention-samba4
      MarkInstall univention-samba-local-config:amd64 < none -> 14.0.5-6A~5.0.0.202204260244 @un uN > FU=0
    Installing univention-samba4-sysvol-sync as Depends of univention-samba4
      MarkInstall univention-samba4-sysvol-sync:amd64 < none -> 9.0.8-2A~5.0.0.202207141233 @un uN > FU=0
    Installing winbind as Depends of univention-samba4
      MarkInstall winbind:amd64 < none -> 2:4.16.2-1A~5.0.0.202207191731 @un uN > FU=0
    Installing univention-monitoring-samba as Recommends of univention-samba4
      MarkInstall univention-monitoring-samba:amd64 < none -> 1.0.0-6A~5.0.0.202207221052 @un uN > FU=0
    Installing univention-nagios-samba as Recommends of univention-samba4
      MarkInstall univention-nagios-samba:amd64 < none -> 5.0.1-1A~5.0.0.202206231931 @un uN > FU=0
  Installing sqlite3 as Recommends of univention-s4-connector
    MarkInstall sqlite3:amd64 < none -> 3.27.2-3+deb10u1 @un uN > FU=0
  Installing univention-monitoring-s4-connector as Recommends of univention-s4-connector
    MarkInstall univention-monitoring-s4-connector:amd64 < none -> 1.0.0-6A~5.0.0.202207221052 @un uN > FU=0
  Installing univention-nagios-s4-connector as Recommends of univention-s4-connector
    MarkInstall univention-nagios-s4-connector:amd64 < none -> 5.0.1-1A~5.0.0.202206231929 @un uN > FU=0
```

For each package a line consisting of multiple fields is printed by [apt-pkg/prettyprinters.cc](https://github.com/Debian/apt/blob/main/apt-pkg/prettyprinters.cc#L19):
- the package name
- `<`: start of package details
- the version of the currently installed package version or `none`.
- `->` Optional *to-be-installed* package version.
- `|` Optional candidate package version if different then installed to to-be-installed version.
- `@` the short status of the package, consisting of 2-3 characters describing the current and desired state:
  - Selected state:
    - `u`: unknown
    - `i`: install
    - `h`: hold
    - `r`: remove
    - `p`: purge
    - `X`: other
  - Installation state:
    - `R`: re-installation required
    - `H`: hold
    - `HR`: hold and re-installation required
    - `X`: other
  - Current state:
    - `n`: not installed.
    - `c`: configuration files
    - `H`: half installed
    - `U`: unpacked
    - `F`: half configured
    - `W`: triggers awaited
    - `T`: triggers pending
    - `i`: installed
    - `X`: other
- `p`: Protected
- `r`: re-install
- `u`: Upgradable
- `m`: Marked
- `g`: Garbage
- State:
  - `N`: new installation
  - `U`: upgrade
  - `D`: downgrade
  - `I`: install
  - `P`: purge
  - `R`: remove
  - `H`: held
  - `K`: keep
- Now Broken state:
  - `Nb`: now broken
  - `NPb`: now policy broken
- Install broken state:
  - `Ib`: installation broken
  - `IPb`: installation policy broken
- `>` end of package details
- `FU=1` for packages named on the command line, otherwise `FU=0`.

## Conflict resolution phase

```
Starting pkgProblemResolver with broken count: 1
```
By just marking the given package as *to be installed*, the dependencies are then *broken* as not all dependencies are satisfied.
Here one package is broken, which the resolver must now fix.

```
Settings used to calculate pkgProblemResolver::Scores::
  Required => 3
  Important => 2
  Standard => 1
  Optional => -1
  Extra => -2
  Essentials => 100
  InstalledAndNotObsolete => 1
  Pre-Depends => 1
  Depends => 1
  Recommends => 1
  Suggests => 0
  Conflicts => -1
  Breaks => -1
  Replaces => 0
  Obsoletes => 0
  Enhances => 0
  AddProtected => 10000
  AddEssential => 5000
```
All packages get a score, which matches how important they are: The more packages depends on it, the higher their score is.
Packages are also boosted by their `Importance` and `Essential` status.
As one package is to be installed via the command line they get the `AddProtected` boost; otherwise the resolver would resolve the conflict by not installing them in the first place.

```
Show Scores
9999 univention-s4-connector:amd64 < none -> 14.0.10-2A~5.0.0.202207141231 @un puN >
...
35 python3-ldb:amd64 < 2:2.5.1-1A~5.0.0.202206171844 | 2:2.5.2-1A~5.0.0.202207191717 @ii umH >
...
30 libldb2:amd64 < 2:2.5.1-1A~5.0.0.202206171844 | 2:2.5.2-1A~5.0.0.202207191717 @ii umH >
...
8 samba-dsdb-modules:amd64 < 2:4.16.2-1A~5.0.0.202206271026 -> 2:4.16.2-1A~5.0.0.202207191731 @ii umU Ib >
...
```
All installed and to-be-installed packages are listed here.
Each line starts with the calculated score for the package.

```
Starting 2 pkgProblemResolver with broken count: 1
```
This is the same line as above, with just the `2` inserted to distinguish those two copy and pasted lines.

```
Investigating (0) samba-dsdb-modules:amd64 < 2:4.16.2-1A~5.0.0.202206271026 -> 2:4.16.2-1A~5.0.0.202207191731 @ii umU Ib >
```
- The package `univention-samba4` depends on `samba-dsdb-modules`, which APT claims to be unresolved.
- Actually version `2:4.16.2-1A~5.0.0.202206271026` is installed but `2:4.16.2-1A~5.0.0.202207191731` is to be installed.
  the `->` indicates a schedules action to update it; `|` would be used to *inform* you instead that the package is either currently already installed or to be installed.
- The package should be installed and actually is installed (`@ii`).
- The package is upgradable `u`, marked `m` and currently scheduled for upgrade `U`.
- The installation status us currently broken `Ib`.

```
Broken samba-dsdb-modules:amd64 Depends on libldb2:amd64 < 2:2.5.1-1A~5.0.0.202206171844 | 2:2.5.2-1A~5.0.0.202207191717 @ii umH > (> 2:2.5.2~)
```
- The package `samba-dsdb-modules` itself depends on a newer version of `libldb2 (> 2:2.5.2~)`.
- Currently version `2:2.5.1-1A~5.0.0.202206171844` is installed.
- But a candidate `2:2.5.2-1A~5.0.0.202207191717` is available.
- The package is upgradable `u`, marked `m` but currently held `H`.

```
  Considering libldb2:amd64 30 as a solution to samba-dsdb-modules:amd64 8
```
As the package `libldb2` has a higher priority `30` over the more low-level package `samba-dsdb-modules` with only priority `8` the resolver decides to upgrade that dependency.
```
  Re-Instated libldb2:amd64
  Re-Instated samba-dsdb-modules:amd64
```
The resolver decides to schedule upgrades for these two packages as it declares `Breaks:` on them.

```
Investigating (1) python3-ldb:amd64 < 2:2.5.1-1A~5.0.0.202206171844 | 2:2.5.2-1A~5.0.0.202207191717 @ii umH Ib >
```
Upgrading `libdb2` alone violates the constraint of `python3-ldb`, which depends on the exact same version: `python3-ldb: Depends: libdb2 (= ${binary:Version})`.
- Currently version `2:2.5.1-1A~5.0.0.202206171844` is installed.
- Version `2:2.5.2-1A~5.0.0.202207191717` is available, but (currently) **not** scheduled for installation: notice the `|` here instead of `->`!
- The package should be installed and actually is installed (`@ii`).
- The package is upgradable `u`, marked `m` but currently held `H`.
- The package is in the *install broken state*, which needs fixing next.

```
Broken python3-ldb:amd64 Depends on libldb2:amd64 < 2:2.5.1-1A~5.0.0.202206171844 -> 2:2.5.2-1A~5.0.0.202207191717 @ii umU > (= 2:2.5.1-1A~5.0.0.202206171844)
```
Due to the pending upgrade of `libdb2` package `python3-ldb` is now broken.
```
  Considering libldb2:amd64 30 as a solution to python3-ldb:amd64 35
```
Because the priority of `python3-ldb` with 35 is higher than priority 30 for `libldb2`, the resolver decides to keep `python3-ldb` in the current state.
For that it cancels the upgrade of `libldb2`, which re-established the constraint on the same version match.
```
  Added libldb2:amd64 to the remove list
  Fixing python3-ldb:amd64 via keep of libldb2:amd64
```
The canceled upgrade of `libdb2` bubbles up the chain and the installation command aborts with an error.

# Fixes

There are multiple *fixes*, to get the installation working.

1. It can be fixed by adding `libdb2` manually to the `apt-get install` command.
   The will add `libdb2` with score `10000-2 = 9998` to the list, forcing an update.

2. `univention-samba4` can add a dependency on `samba-dsdb-modules (>= 2:4.16.2-1A~5.0.0.202207191731),`.
   This explicitly pulls in the newer version, which then also pulls in newer versions of `libldb2` and `python3-ldb`.

3. Add a `libldb2: Breaks: python3-ldb (<< ${binary:Version})`.
   When upgrading `libldb2` is considered, this will also schedule an upgrade of `python3-ldb`, which will then will be in lock-step again.

According to my gut feeling I think 3 is the most correct one, but if you know better: please mail me.

# Post Script

After publishing this and also asking the [APT Team](https://lists.debian.org/deity/2022/07/msg00033.html) _Andre Wagner_ mailed me this link: [An Ubuntu 22.04 LTS Fix Is Coming For A Very Annoying & Serious APT Problem](https://www.phoronix.com/news/Ubuntu-22.04-APT-Breaks-Things)
It points to an [Ubuntu bug report](https://www.phoronix.com/news/Ubuntu-22.04-APT-Breaks-Things) and includes [a patch](https://salsa.debian.org/apt-team/apt/-/merge_requests/248), which was [merged](https://salsa.debian.org/apt-team/apt/-/merge_requests/248) 2 weeks ago into [apt](https://salsa.debian.org/apt-team/apt/-/merge_requests/248).

# Disclaimer

Please note that there are different tools, which might use different resolvers:
- `apt-get` is the stable command line tool, which should be used from scripts.
- `aptitude` provides an interactive text user interface, but can also be used from scripts.
  It can be used interactively to resolve conflicts, but requires some extra knowledge.
- `apt` is a new tool for interactive usage: It combines functions from other tools like `apt-cache` and `apt-mark`, but is not yet considered stable and should not be used in scripts.

{% include abbreviations.md %}
