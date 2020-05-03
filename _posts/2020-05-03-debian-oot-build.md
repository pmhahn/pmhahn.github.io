---
layout: post
title: "Build Debian packages out-of-tree"
date: 2020-05-03 07:40:00  +0200
categories: debian
excerpt_separator: <!--more-->
---

Debians `dpkg-buildpackage` has the annoying feature, that the build artifacts are placed in the parent directory.
[Bug 657491](https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=657401) requested that feature in 2012, but there still is no such option.
While working on [speeding up the build process]({% post_url 2020-05-02-speedup-debian-package-build %}) I investigated, how that could be fixed or at least be worked around.

<!--more-->

Building a package by default involves the following programs calling each other:
* `debuild`
  * `dpkg-buildpackage`
    * `dpkg-source --build $dir` → `../$pkg_$ver.dsc`
    * `make -f debian/rules`
      * `dpkg-deb --build $dir $deb`
    * `dpkg-genbuildinfo` → `../$pkg_$ver_$arch.buildinfo`
    * `dpkg-genchanges` → `../$pkg_$ver_$arch.changes`

Several of them already have an option to overwrite the directory and/or an be invoked from paths outside the normal build directory:

* `dpkg-source` places the files in the current working directory by default, but you can give it an absolute path.
* `dpkg-deb` can use an absolute path for its output file.
   It is invoked by `dh_builddeb`, which has the option `--destdir=Verzeichnis`.
* `dpkg-genbuildinfo` has the `-u$dir` option.
* `dpkg-genchanges` has the `-u$dir` option.

So for first try for an *out-of-tree* build we can do it like this:

```sh
#/bin/sh
set -e -u

src="$PWD"
out="$HOME/my-repo"

# Manually build source with absolute path
dpkg-source --before-build .
(cd "$out" && dpkg-source --build "$src")

# For the rest use dpkg-buildpackage
DH_OPTIONS="--destdir=$out" \
dpkg-buildpackage \
  --buildinfo-option=-u"$out" \
  --changes-option=-u"$out" \
  -b --no-sign
dpkg-source --aftere-build .
```
This *mostly* works but the path for the `.chagnes` file is hard-coded in `dpkg-buildpackage` and cannot be changed.
For signing you also would need to call `debsign` on the `.changes` files (or the individual parts `.dsc`, `.buldinfo`)

Using `DH_OPTIONS` also is **not** a good idea as the option `--destdir` is not unique for `dh_builddeb`:
It is also used by `dh_auto_install` so files get installed in the wrong location.

TBC...
