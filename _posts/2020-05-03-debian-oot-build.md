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

So for a first try for an *out-of-tree* build we can do it like this:

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

# More work like
dpkg-source --aftere-build .
(cd "$out" && debsign *.changes)  # FIXME
```

This *mostly* works but **fails** for the following reasons at the moment:

1. The path for the `.changes` file is hard-coded in `dpkg-buildpackage` and cannot be changed.

2. Using `DH_OPTIONS` also is **not** a good idea as the option `--destdir` is not unique for `dh_builddeb`:
   It is also used by `dh_auto_install` so files get installed in the wrong location.

3. [Debian policy](https://www.debian.org/doc/debian-policy/ch-source.html#main-building-script-debian-rules) only **recommends** the use of `debhelper`, but packages are free to use other strategies in their file `debian/rules`.
   Instead of using the wrapper `dh_builddeb` packages may use `dpkg-deb` directly and pass arbitrary paths.
   Therefore there is no way for `dpkg-buildpackage` to force an `--output-directory` to `dpkg-deb` without the risk of breaking some obscure package.

TBC…
