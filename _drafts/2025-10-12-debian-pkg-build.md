---
title: 'Debian Package building'
date: '2025-10-12T14:36:00+02:00'
layout: post
categories: debian
---

# How to build Debian packages

I'm a Debian Developer (DD) since 2002.

## On host build

`debian/rules` `build` `binary`.
`dpkg-buildpackage`
`debuild`

- Requires `sudo` privileges.
- Host system cluttered with many packages of the time.
- Build environment is not minimal as usually more packages are available than specified in `Build-Depends` from `debian/control`.
- Fast: Once all packages are installed, it is easy to do multiple builds and debug build failures.
- Installing experimental build dependencies might reduce system stability.

## `chroot`

```console
$ sudo debootstrap
```

Or modern `cdebootstrap` or `mmdebstrap`.

- Requires `sudo` privileges.
- Care must be taken to not start services in `chroot` on installation. Setup `/sbin/policy-rc.d`!
- Fast: Once all packages are installed, it is easy to do multiple builds and debug build failures.
- Requires manual maintenance work to remove old environments or re-setup them for each new build.

## `pbuilder`

Uses compressed tape archives.
Extracts a private build environments for each build.

`pdebuild` is a replacement for `debuild` using `pbuilder`.

```console
$ pdebuild
```

- Requires `sudo` privileges.
- Stores pristine build environment in TAR files to save space.
- Wraps create, update, extract and cleanup.
- Just shell code.
- Installs any additional packages from scratch.

## `pbuilder --use-pdebuild-internal`

`debian/rules clean` is called before the build to restore a pristine build environment.
It removes any artifacts from previous builds.
Normally `pbuilder` runs this **ouside** the `chroot` environment.
This is a problem if running `clean` already requires some build dependencies, e.g. additional dehlepers.

Therefore `pbuild` implements an alternative method where `clean` runs **inside** the `chroot` environment.
The working directory is bind-mounted inside the `chroot` environment.
Inside the `chroot` environment a user is created to match the UID of the invoking user.
That way the files created inside the environment are later owned by the invoking user.

- Solves the problem of running `clean` on the host
- Installs `pbuilder` and all its dependencies each time into the `chroot` – the environment is no longer minial.

## `cowbuilder`

Uses hard-links to create private build environments from a master environment.
Install a `LD_PRELOAD` library from `cowdancer` to intercept write access to any files and breaks hard-links then.

- The `LD_PRELOAD` wrapper might fail in some cases. In the worst case the original files are modified.

## `btrfsbuilder`

Use the snapshot-feature of BTRFS to create a private volume for each build.

- Only works with BTRFS
- Not officially backaged

## `whalebuilder`

Docker

## `debocker`

Docker

## `qemubuilder`

This uses virtual machines runnign with Qemu.

- Allows building **and running** binaries for foreign architectures.

## `sbuild`

```console
```

## `sbuild-qemu`

```console
```

- Allows building **and running** binaries for foreign architectures.
