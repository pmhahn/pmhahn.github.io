---
layout: post
title: "Reproducible Build"
date: 2019-04-09 17:54:00  +0200
categories: debian
---

[Debian](https://www.debian.org/) had started to make their build reproducible:
Two builds of the same source package should produce bit identical binary packages.
This allows anybody to verify that nobody tempered with the build system.

More Linux and BSD distributions joined that effort.
For that the project was later moved to <https://reproducible-builds.org/>.

Build date
==========
Many packages include timestamps into the build.
For that the environment variable `SOURCE_DATE_EPOCH` can be set to a number of seconds since the UNIX epoch.
Several tolls then use that fixed date instead of the current time.
<https://reproducible-builds.org/docs/source-date-epoch/> describes this in more detail.

Apache FOP
==========
[FOP](https://xmlgraphics.apache.org/fop/) is a tool to convert an XML description into PDF.
It embeds the current time as `/CreationDate` inside any generated PDF file.

[Apache FOP](https://issues.apache.org/jira/browse/FOP-2854) has declined the request to add an overwrite for `CreationDate`.
Here's a patch to overwrite it using said environment variable.

`faketime` cannot be used as otherwise the process will stall.
To me this looks like FOP or Java does some kind of busy-waiting.o

I tries to add support for `SOURCE_DATE_EPOCH` into the source, but so far have failed.
I need to find some time to continue that.
