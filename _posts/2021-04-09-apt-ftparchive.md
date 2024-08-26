---
layout: post
title: "Faster Debian packages indexing"
date: 2021-04-09 13:59:00  +0200
categories: debian
excerpt_separator: <!--more-->
---

You can install Debian packages using `dpkg -i $pkg.deb`, but this low-level tool does not resolve inter-package dependencies.
This is the job of APT, the *Advanced Packaging Tool*.
It usually works on a set of packages, which are shipped in a [package repository](https://wiki.debian.org/DebianRepository/Format).
This has a `Packages` file, which lists all binary packages included in the repository.
Basically it contains the concatenated package meta data from all packages.

<!--more-->

# Tools

## `dpkg`

Basically you can get the meta data from `dpkg -I $pkg.deb` or `dkg -s $pkg`.
For multiple packages you have to iterate that for each package.

## `dpkg-scanpackages`

There even is [dpkg-scanpackages](man:dpkg-scanpackages), which does this by scanning the given directory recursively.

In addition it supports the so called *override* files:
They are needed to overwrite the meta data of packages **after** a package has already been built.
This usually happens with new Debian releases, when the package did not change but needs updated meta data.
So instead of just re-rebuilding the package just for that the `Section` or `Priority` can be changed easily.

For source packages there is `dpkg-scansources`, which is used to generates the `Sources` files.

## `apt-ftparchive`

[apt-ftparchive](man:apt-ftparchive) is an improved version, which can do much more.
In combines `dpkg-scanpackages` with `dpkg-scansources`, but also can generate the `Releases` file.
In addition to all those `Packages` and `Sources` files it may also list other files, for example translation or icon files.
The file is often associated wit a `Releases.gpg` file containing the GnuPG signature required for checking the security chain.
Newer releases are using a `InRelease` files, which contains the signature inline to allow atomic testing.

# Caching

On top of the `apt-ftparchive` also adds caching.
Basically all tools from above still use `dpkg -I` to do the heavy listing.
Forking such a process for a large number of packages will make this inefficient very fast.

So the cache stores that data for each *path*.
When next time the index is re-built, the meta data from the cache is used if available.

The cache uses a simple *Berkeley database*.
The file can be specified with `-d` / `--db` / `-o APT::FTPArchive::DB`.

## Uniqueness

For Debian package repositories there is a very important rule:

> In ideal world, tuple (architecture, name, version) should identify unique package.

This allows tools to uniquely identify packages using that triple, which form the package file name: `${name}_${version}_${architecture}.${tyype}`
Many tools use this invariant and **break badly** if the name is **reused** for a file with **different** content.
This easily happens if a package is re-built, bit is not [reproducible](https://reproducible-builds.org/).

In that case `apt-ftparchive` would re-used the **old** meta-data for the **new** content.
This usually breaks when such a package is first downloaded by a client as then the file checksums no longer match!

## Invalidation

This can be solved in two ways:

1. Make sure to **never** re-use the triple respective the filename.
2. Invoke `apt-ftpachive` with `-o APT::FTPArchive::AlwaysStat=true`: This will store the package files modification time-stamp with the cache entry. The cached entry is then only used if the files time-stamp is still the same.

## Performance issues

Enabling `AlwaysStat` may create a performance problem:
For each binary package file `apt-ftparchive` now needs to do a `stat()` call to get the I-node information.
The Linux kernel will cache that data internally, but doing this for 58k with a cold cache will take some time.
This get much worse if you do this over NFS as there each `stat()` call takes a round-trip to the server.

Without `AlwaysStat` only a single `listdir()` call per directory *should* be needed.
Everything else only requires looking up the returned file names in the cache, which mostly happens in memory.

## File Tree Walk

Actually you will notice that `apt-ftparchive` performs abyssal in the cold-cache case.
It still does `stat()` calls for all files even when `AlwayStat` is disabled.
The culprit here is [ftw()](man:ftw(3)], the C-library used to implement the *File Tree Walk*:

To **walk** the directory recursively it must check the type of the returned directory entry:
* For a *directory* it must recurs.
* For a *file* it must do the lookup thing.
* *Symbolic links* may be skipped or followed.
* Other types like *device files*, *UNIX sockets*, *named PIPEs* should be ignored.

## `d_type`

You will find using `find -type f` performing a lot better even in the cold-cache case.
Normally a *directory entry* just maps the *name* to the *i-node number*.
But actually most Linux file systems nowadays implement a performance optimization:
The also store the *i-node type* **directly within** the *directory entry* itself.
This is then returned by [readdir()](man:readdir(3)) as `d_type`.
If present the call to [lstat()](man:lstat(2)) can be skipped.

Combining this with `AlwaysStat=false` makes `apt-ftparchive` really fast.
But as `ftw()` is used internally you have to replace this with your own `find`.

# Fast `apt-ftparchive`

Instead you can give `apt-ftparchive` list of files to prevent it from using `ftw()` itself.
But this does not work with `packages` and `sources`, but only with `generate`.
Depending on the file type you specify those files using `FileList` and `SourceFileList`.
They work in the sections `TreeDefault`, `Tree` and `BinDirectory`.

## Build file lists

Depending on your desired output format you have to separate the files by architecture and (micro) type manually:

```sh
find amd64 all source -maxdepth 1 \
  -name \*_amd64.deb  -fprint .files/amd64.apt -fprint .files/amd64.deb  , \
  -name \*_amd64.udeb -fprint .files/amd64.apt -fprint .files/amd64.udeb , \
  -name \*_all.deb    -fprint .files/all.apt   -fprint .files/amd64.deb  , \
  -name \*_all.udeb   -fprint .files/all.apt   -fprint .files/amd64.udeb , \
  -name \*.dsc        -fprint .files/source.dsc
```

## Common configuration

Put this in and the following sections into a `dist.conf` file:

```
Dir {
	ArchiveDir ".";
	OverrideDir ".override/";
	CacheDir ".cache/";
	FileListDir ".files/";
};
```

It is te be used with `apt-ftparchive release dist.conf` then.

You have to manually create some files:

```sh
mkdir -p dists/dist/main/binary-amd64
mkdir -p dists/dist/main/source
mkdir -p dists/dist/main/debian-installer/binary-amd64
```

## Build flat Packages file

Build a `Packages` or `Sources` file to be included with `deb [trusted=yes] file:///.../ amd64/` and `deb ... all/`:

```
BinDirectory "amd64" {
	// InternalPrefix "<PREFIX>/";
	// BinOverride "";
	// SrcOverride "";
	// ExtraOverride "";
	// SrcExtraOverride "";
	Packages "amd64/Packages";
	BinCacheDB "db.amd64";
	FileList "amd64.apt";
};
BinDirectory "all" {
	Packages "all/Packages";
	BinCacheDB "db.all";
	FileList "all.apt";
};
BinDirectory "source" {
	Sources "source/Sources";
	SrcCacheDB "db.source";
	FileList "source.dsc";
};
```

## Build dists Packages file

Build a `Packages` or `Sources` file to be included with `deb [trusted=yes] file:///.../ dist main`:

```
TreeDefault {
	BinCacheDB "db.$(ARCH)";
	SrcCacheDB "db.$(ARCH)";
	FileList "$(ARCH).deb";
	SourceFileList "$(ARCH).dsc";
};
Tree "dists/dist" {
	Sections "main";  // contrib non-free
	Architectures "amd64 source";
	SrcDirectory "source/";
	Packages::Extensions ".deb";
};
Tree "dists/dist/" {
	Sections "main";  // contrib non-free
	Architectures "amd64";
	FileList "$(ARCH).udeb";
	Packages::Extensions ".udeb";
	Packages "$(DIST)/$(SECTION)/debian-installer/binary-$(ARCH)/Packages";
};
```

# Summary

1. Do not use `ftw()` on large directories.
2. In Python use [scandir()](https://docs.python.org/3/library/os.html#os.scandir) instead of [listdir()](https://docs.python.org/3/library/os.html#os.listdir).
3. `apt-ftparchive` still does a lot of `readlink()` calls, which need more investigation.
4. The cache lookup used the path as given; make sure to not prefix it with `./` only in same cases as this leads to duplicate cache entries.

# Appendix

There are some other knobs for tuning:

## Hash algorithms

Calculating the different hash sums takes time.
You can enable / disable them individually by specifying the following options:

* `apt::ftparchive::md5 "<BOOL>";`
* `apt::ftparchive::sha1 "<BOOL>";`
* `apt::ftparchive::sha256 "<BOOL>";`
* `apt::ftparchive::sha512 "<BOOL>";`

This can also be configured for `Packages` and `Sources` individually:

* `apt::ftparchive::packages::<ALGO> "<BOOL>";`
* `apt::ftparchive::sources::<ALGO> "<BOOL>";`

## Compression formats

You can also configure the compression formats:

* The uncompressed files
* `.gzip`
* `.bzip2`
* `.lz4`
* `.lzma`
* `.zstd`
* `.xz`

## Contents

Each binary package ships directories and files.
You can get their paths from `dpkg -c $pkg.deb` or `dpkg -L $pkg` for a single package.
These paths are collected in the file `Contents`.
This is useful is you want to know which packages ship which files.

Extracting this data, storing it in the cache, putting it into the file and compressing it takes some time.
This must be enabled explicitly with `--contents` or `-o apt::ftparchive::contents=true`.

## Internals: Cache format

The format of the cache file is an internal detail of `apt-ftparchive`.
Normally you should not use it yourself, but knowing the format helps with debugging.

The *Berkeley database* contains several entries per *file path*.
Their *key* is build by appending a `:` and a two-letter code depending on the type to the path:

*   `:st` (stat):
    File statistics for all files like `.deb`, `.udeb`, `.dsc`, `.orig.tar.gz`, `.debian.diff.gz`, `.debian.tar.gz`.
    It uses the file time of last modification `mtime` for cache validation.
    The file size `size` and calculated hashes using `md5`, `sha1`, `sha256` and `sha512` are stored within.
    Additional `flags` indicate, if other database records exists.

    Be aware that this entry is architecture and version dependant!
    As of 2021 it is 152 bytes for `x86_64`.

*   `:cl` (Control):
    Debian binary package control data for binary packages `.deb` and `.udeb`.
    This is used to build the `Packages` files.

*   `:cn` (Content):
    Debian binary package content list of files.
    This is used to build the `Contents` files.

*   `:cs` (Source):
    Debian source package control data `.dsc`.
    This is used to build the `Sources` files.

*[APT]: Advanced Packaging Tool
