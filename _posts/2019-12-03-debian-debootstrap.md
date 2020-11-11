---
layout: post
title: "Minimal Debian images"
date: 2019-12-03 09:30:00  +0200
categories: linux debian filesystem
excerpt_separator: <!--more-->
---

At work I need minimal Docker images.
`debootstrap` is Debians default way to create `chroot` environments.
By default they include all *required* and *essential* packages.
For a hardware system this is okay, but too much for a container image.

<!--more-->

Using a command line the following will print the binary packages, which are installed by default:

	grep-dctrl -n -s Package -F Essential yes -o -F Priority required \
		/var/lib/apt/lists/deb.debian.org_debian_dists_buster_main_binary-amd64_Packages

As an alternative you can use `debootstrap` directly to get a list of packages.
This also includes the resolved list of dependent packages:

	debootstrap --print-debs \
		sid \
		"${TMPDIR:/tmp}/deb.sid" \
		http://deb.debian.org/debian/

This includes packages like:

* `diffutils`
* `e2fsprogs`
* `fdisk`
* `login`

The first two can be removed easily, but the last two are still pulled in as required dependencies.

So to build a minimize `chroot` environment (for `pbuilder`) I use the following command:

	pbuilder create \
		--basetgz base-sid-amd64.tgz \
		--distribution sid \
		--mirror http://deb.debian.org/debian/ \
		--architecture amd64 \
		--debootstrapopts --variant=buildd \
		--debootstrapopts --exclude=e2fsprogs,diffutils \
		--extrapackages pbuilder

(I install `pbuilder` inside the `chroot` environment as I always use `--pbuilder-internal` to resolve dependencies inside the `chroot` environment.)

More less
=========

This image is still quiet large.
Please have a look at [Debuerro](https://github.com/debuerreotype/debuerreotype) which creates the official *Slim* [Debian Docker Images](https://hub.docker.com/_/debian).
They use the following [tricks to shrinkt](https://github.com/debuerreotype/debuerreotype/blob/master/scripts/debuerreotype-minimizing-config) the image even more compared to my naive approach:

* Setup `/etc/dpkg/dpkg.cfg.d/` to exclude certain files during package installation:
  * All documentation below `/usr/share/doc/` except the `copyright` files.
  * All locale related files below /usr/share/locale/` except for `C.UTF-8`.
  * The unpacked `Packages`, `Sources`, `Release` files below `/var/lib/apt/`.
