---
layout: post
title: "Speedup Debian package building"
date: 2020-05-02 11:00:00  +0200
categories: debian filesystem virt UCS
excerpt_separator: <!--more-->
---

For my employee [Univention](https://www.univention.de/) I want to setup a Continuous Integration system to build Debian packages.
We have been using our own build-system called *Repo-NG* based on [pbuilder](https://pbuilder-team.pages.debian.net/pbuilder/), which show their age.
I tried several tricks to improve the build speed as we have to build many packages and many of them multiple times.

<!--more-->

# Improving pbuilder speed

So far we have been using *pbuilder*, which in its initial form uses *compressed tape archives* `.tar.gz` to have clean build environments.
For each build they are extracted to a new location, in which the package build happens.
This is slow when using slow disk and was my first vector for optimization.

## Using unsafe dpkg

Newer `dpkg` has an option to disable its synchronization:

```sh
echo "force-unsafe-io" > /etc/dpkg/dpkg.cfg.d/force-unsafe-io
```

This is already used during the initial setup, but removed after the Debian installer has finished.
Enabling it in throw-away build environments is a first optimization.

## Using Qemu `cache=unsafe`

When we moved *Repo-NG* from physical servers into *virtual machines*, we gave each VM a *stratch volume* using Qemus `cache=unsafe` feature.
This filters our all `sync()` calls to flush the data to disk, which greatly improves the time to setup the required build dependencies as `sync()` is used a lot during `dpkg` installing packages.

## Using Eat-my-data

An alternative it to use [eat-my-data](https://launchpad.net/libeatmydata).
This works quiet well most of the times, but I remember having problems with some packages, which failed to build.
My main concern is that I have to install that library into each build environment, which then is no longer minimal.

## Using SECCOMP

Another alternative is using [SECCOMP to filter out sync operations](https://bblank.thinkmo.de/using-seccomp-to-filter-sync-operations.html).
This also works quiet well but some integration tests fail, which notice a discrepancy between requested and actual sync mode.
This also breaks when doing cross-compiles, but other has the benefit, that I can setup it outside the build environment.

## Using tmpfs

For our [piuparts](https://wiki.debian.org/piuparts) system I'm using a very large *tmpfs* file-system backed by lots of swap space.
As long as everything fits into RAM it's ultra-fast as everything happens in RAM only most of the time.

## Using btrfs

On my personal systems I have been using [btrfs](https://btrfs.wiki.kernel.org/index.php/Main_Page) with [btrfsbuilder](https://github.com/koalatux/btrfsbuilder).
The idea is to use btrfs built-in feature *writeable snapshots* instead of using extracting `tar` files all the time.
This works well with fast disks, but the work to manage the meta-data costs lots of time:
As btrfs must be prepared to be consistent in case of crashes, it uses `sync()` a lot, which makes it slow.
Combining it with one of the other methods to reduce the `sync()`-load improves this.

## Using overlayfs

[OverlayFS](https://www.kernel.org/doc/Documentation/filesystems/overlayfs.txt) is the successor of [UnionFS](https://unionfs.filesystems.org/) and [auFS](http://aufs.sourceforge.net/).
It allows to stack multiple file systems over each other so that changes go to the top-level file system, but all unchanged files show through.

[sbuild](https://wiki.debian.org/sbuild#sbuild_overlays_in_tmpfs) already describes a setup, where the base system is kept unpacked in the file system and each built gets a `tmpfs` overlay for the data.
This sounds like an excellent solution as it combines the best of two worlds:
- no time to unpack the tar file each time
- use the ultra fast tmpfs for all temporary build data

I have not yet investigated this, but it is on my to-do list.
In the past [btrfs could not be used with overlayfs](https://btrfs.wiki.kernel.org/index.php/Changelog), but this is fixed since Linux Kernel version 4.15.
My idea would be to create a new snapshot for each build in addition to using unionfs, so that I can update the master version any time even while other builds are still running using an old snapshot.

## Variants of pbuilder

There is [cow-builder](https://wiki.debian.org/cowbuilder), which also uses already extracted build environments.
It then uses an `LD_PRELOAD` wrapper to implement `Copy-on-write` in user-space, which works quiet well.
Personally I don't like this approach I feat corruption:
If the library does not intercept all calls correctly or a process by-passes the library, changes might leak back into the master environment and taint all following builds.

Another variant is [qemu-builder](https://wiki.debian.org/qemubuilder), which uses Qemu to setup a virtual machine for each build.
Qemu has a built-in *snapshot* feature to protect the master image from changes:
All changes go into an overlay image, which can be placed on tmpfs for extra speed.
Using Qemu also allows cross-building for other architectures.

# Using docker

[Docker](https://www.docker.com/) can also be used to build Debian packages.
There already is [Whalebuilder](https://btrfs.wiki.kernel.org/index.php/Changelog).

Multiple things make Docker attractive:
- it is easy to create a clean environment with just a single command
- it already implements caching

If you build many packages multiple times, most of the time goes into preparing the build environment:
- determine, which packages are required
- downloading them
- installing them
- configuring them

This first step to reduce the time needed for that is to create docker images for building packages.
For each UCS release I have multiple docker images:

1. The `minbase` version consisting of *Essential: yes* only.
   Currently this image is created by doing a `debootstrap --variant=minbase`.
   Doing it this way gets you an image, which  still includes lots of extra stuff not needed for Docker.
   In the feature I want to adapt the [approach from slim images](https://ownyourbits.com/2017/02/19/creating-a-minimal-debian-container-for-docker/).

2. The `devbase` version has `build-essential` already installed.
   This includes the `gcc` compiler and `dpkg-dev` package, which are always required for building Debian packages.

Those images need to be re-build each time one of the packages installed in the image gets an update.
It's on my to-do list to automate this in the future.

## Ad-hoc

The ad-hoc approach for building packages is like this:

```sh
docker run --rm -ti -v "$PWD":/work -w /work "$base_image" sh
apt-get -qq update  # optional: Update Packages files
apt-get -qq upgrade   # optional: Upgrade already installed packages
apt-get -qq build-dep .  # Install build dependencies
dpkg-buildpackage -uc -ub -b  # build package
exit 0
```

This naive approach has the following drawbacks:
- you run the build as the user `root` inside the docker container.
  You have to call `adduser` and `su` to create them inside the container to fix this.
- the time and network bandwidth to download and install the dependencies is wasted afterwards.
  They have to be re-done for each new build.
- the build artifacts are lost as they are put outside your current working directory.
  You need to setup a second volume for the parent directory to receive the artifacts.

```sh
docker run \
  --add-host updates.knut.univention.de:192.168.0.10 \
  -v "$TMPDIR":/work \
  -v "$PWD":/work/src \
  -e BUID="$UID" \
  --rm -ti "$base_image" sh
apt-get -qq update  # optional: Update Packages files
apt-get -qq upgrade   # optional: Upgrade already installed packages
apt-get -qq build-dep .  # Install build dependencies
adduser --home /work/src --shell /bin/bash --uid $BUID --gecos '' --disabled-password --no-create-home build
exec su -c 'exec dpkg-buildpackage -uc -us -b -rfakeoot' build
```

(I need that `--add-host` argument as my Docker images use that host name for our Debian package repository, but I run that image on my Notebook behind a VPN, so default DNS resolution of that name does not work.)

## With Dockerfile

An alternative is to use something like the following [Dockerfile](https://docs.docker.com/engine/reference/builder/):

```Dockerfile
ARG ucs=latest
FROM docker-registry.knut.univention.de/phahn/ucs-devbase:$ucs AS build
RUN install -o nobody -g nogroup -d /work/src
WORKDIR /work/src
COPY --chown=nobody:nogroup debian/control debian/control
RUN apt-get -qq update && apt-get -qq build-dep . ; apt-get clean
USER nobody
COPY --chown=nobody:nogroup . .
RUN dpkg-buildpackage -uc -us -b -rfakeroot

FROM scratch
COPY --from=build /work/*.deb /
```

The idea is to
1. Use the previously prepared docker image
2. Get the required `Build-Depends` and `Build-InDepends` from `debian/control` and install them.
3. Build the package
4. Copy the built binary packages into an empty new Docker image.

I can use it like this from the directory containing my extracted source tree:

```sh
docker build --add-host updates.knut.univention.de:192.168.0.10 -t deb -f ~/misc/Docker/Dockerfile.build .
docker image save deb | tar -x -f - -O --wildcards '*/layer.tar' | tar tf -
```

This works quiet well as Dockers built-in caching mechanism is used to cache the build environment.
While `debian/control` does not change there is no need to set it up again in most cases.
What's missing here is again the tracking of changed packages in that image.

# Statistics

Here's some old performance data collected from compiling Linux Kernel 3.2.39.
The system had an Intel Core i7 with 8 GiB RAM and a single 500 GiB SATA disk.
A 100 GiB LVM volume was either used as a `ext4` file system or an additional swap space.

|         | real | user | sys  | real  | user   | sys  |
|---------|------|------|------|-------|--------|------|
| ext4    | 0:19 | 0:10 | 0:02 | 42:47 | 168:21 | 8:08 |
| seccomp | 0:09 | 0:08 | 0:02 | 66:20 | 206:05 | 8:14 |
| tmpfs   | 0:14 | 0:10 | 0:01 | 43:52 | 168:36 | 8:15 |

The data above was a little bit surprising, since I expected `tmpfs` to perform better.
My reading is that only the unpack time improved, because there most operations are performed in RAM.
The compile itself performed worse, since running 8 compilers in parallel needs lots of RAM, which creates memory pressure and forces lots of data to be transferred between RAM and the hard-disk.

This needs more investigation, especially with smaller packages and using less parallelism.

# Replacing pbuilder

`pbuilder` shows its age: It's a collection of shell script having multiple issues.
Despite the name `pbuilder` prepares the *source package* on your host system, for which it installs all the build-dependencies *on your host*!
You can disable that by using `--use-pdebuild-internal`, but this is *not* the default!
As soon as you use that you get a new bunch of shell quoting errors, as many more things then need to be done inside the `chroot` environment.

Debian's official build system uses [sbuild](https://wiki.debian.org/sbuild), which looks more robust.

# Improving repository services

## Speed up Package indexing

*Repo-NG* uses `apt-ftparchive` internally with lots of trick to get it up to speed.
Most important is the option `--db` to use an database for caching the extracted package data.
Historically we used `dpkg -I` to extract that data into a text file and concatenated those fragments into the resulting `Packages` file.
This happened over NFS and resulted in too many `stat()` calls to implement proper cache invalidation:
The time-stamp for each `.deb` had to be compared to the time-stamp of each cache fragment.

We also switched to `-o APT::FTPArchive::AlwaysStat=false` to further reduce the number of `stat()` calls.
The downside of this is that you have to be very careful, that the triple `(pkg-name, pkg-version, pkg-arch)` is unique for each package and never re-used for a file with different content.
(Good when your packages are [reproducible](https://reproducible-builds.org/), our's are not.)

Another improvement was the use of [Python's os.scandir()](https://www.python.org/dev/peps/pep-0471/) instead of `os.walk()`:
The later does a `stat()` on all files to distinguish files from directories, which added another round of `stat()` calls.
This is very fast while that data is still cached in the [Linux kernels dentry cache](https://www.kernel.org/doc/Documentation/filesystems/vfs.txt) from the last run, but abyssal slow in the morning after `updatedb` trashed that cache.

## Repository hosting

For previous projects I've used [reprepro](https://salsa.debian.org/brlink/reprepro) both personally but also in my company to host packages built by CI.
Currently I'm investigating the move to [Atly](https://www.aptly.info/), which has a very powerful [REST API](https://www.aptly.info/doc/api/).
This allows it to be used via [cURL](https://curl.haxx.se/) until GitLab implements its own [Debian](https://gitlab.com/gitlab-org/gitlab/issues/5835] [Package Registry](https://docs.gitlab.com/ee/user/packages/).

# Summary

I should repeat the compilation test with the different variants.
