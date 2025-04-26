---
layout: post
title: "dpkg --compare-versions"
date: 2021-09-21 12:00:00  +0100
categories: linux debian
tags: version
excerpt_separator: <!--more-->
---

Q: How are Debian package version strings compared?

A: This is mandated by [Debian Policy](https://www.debian.org/doc/debian-policy/ch-controlfields.html#version) and `dpkg` is considered the **single truth** of implementation.

Comparing Debian package version strings is not trivial:
many programs implement this themselves and get it wrong for corner cases — me included.
Therefor use [dpkg --compare-versions]({% post_url 2020-06-11-dpkg-compareversions %}) or one of its wrappers, for example `apt.apt_pkg.version_compare()` for Python or `debversion` for [PostgreSQL](https://salsa.debian.org/postgresql/postgresql-debversion).
Continue reading if you want to understand comparing Debian package version strings yourself, which is important when you increment the version of UCS packages.
The format is:
[_epoch_`:`]_upstream-version_[`-`_debian-revision_]

<!--more-->

1.  The _epoch_ is the last resort when version numbers go backward and [should be used sparingly](https://www.debian.org/doc/debian-policy/ch-controlfields.html#epochs-should-be-used-sparingly).
    It is reserved for cases, for example where upstream changes its version scheme incompatibly or to fix serious errors.
    The _epoch_ is a small number - by default starting at **0** - and it **not** included with the filename:
    This makes comparing packages by filename wrong as this misses the _epoch_!
2.  The _upstream-version_ is a sequence of alphanumeric characters plus the characters _full stop_, _plus_, _hyphen_ and _tilde_.
    The pattern `[0-9A-Za-z.+~-]+?` is **not** _greedy_:
    It will capture **all hyphens except the last one**, which is used to separate the _debian-revision_.
3.  The _debian-revision_ is **optional** and the part after the **last** _hyphen_ character.
    Therefor the same character set as in _upstream-version_ is allowed **except** the _hyphen_ character:
    The pattern `[0-9A-Za-z.+~]+` is greedy.

When two version strings are compared, they are compared section by section, e.g. first numerically by _epoch_, second by _upstrean_version_ and only last by _debian-revision_.
The last two sections require a special algorithm for comparison:

*   Each section is separated into a sequence of consecutive non-digit-characters and digits.
*   Consecutive digits are compared **numerically**
*   All non-digits are compared **lexical** as strings, using a **modified** ASCII encoding
*   `~` sorts before the _empty string_ - think of this as _-ε_ (minus Epsilon), which is often used for _alpha_ / _beta_ / _release-candidates_ or _backports_, as they must sort **before** the final version.
*   After that follow the upper `A-Z` and lower-case letters `a-z`.
*   Last comes the _plus_ `+`, _hyphen_ `-` (only in _upstream-version_) and _full stop_ `.`.

# Examples

I have inserted blanks between the three groups to help you parse the version numbers.

## Examples for parsing

<dl>

<dt><span style="background-color: #efe">1.2</span></dt>
<dd style="background-color: #fee">epoch=0</dd>
<dd style="background-color: #efe">upstream-version=("", 1, ".", 2)</dd>
<dd style="background-color: #eef">debian-revision=("")</dd>

<dt><span style="background-color: #fee">3</span> : <span style="background-color: #efe">1.2</span></dt>
<dd style="background-color: #fee">epoch=3</dd>
<dd style="background-color: #efe">upstream-version=("", 1, ".", 2)</dd>
<dd style="background-color: #eef">debian-revision=("")</dd>

<dt><span style="background-color: #efe">1.2</span> - <span style="background-color: #eef">3</span></dt>
<dd style="background-color: #fee">epoch=0</dd>
<dd style="background-color: #efe">upstream-version=("", 1, ".", 2)</dd>
<dd style="background-color: #eef">debian-revision=("", 3)</dd>

<dt><span style="background-color: #efe">1.2-3</span> - <span style="background-color: #eef">4.5</span></dt>
<dd style="background-color: #fee">epoch=0</dd>
<dd style="background-color: #efe">upstream-version=("", 1, ".", 2, "-", 3)</dd>
<dd style="background-color: #eef">debian-revision=("", 4, ".", 5)</dd>

<dt><span style="background-color: #efe">1</span> - <span style="background-color: #eef">deb9</span></dt>
<dd style="background-color: #fee">epoch=0</dd>
<dd style="background-color: #efe">upstream-version=("", 1)</dd>
<dd style="background-color: #eef">debian-revision=("deb", 9)</dd>

</dl>

## Examples for comparing

<dl>

<dt><span style="color: red;background-color: #efe">1</span> < <span style="color: red;background-color: #efe">2</span></dt>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", <span style="color: red">1</span>)</span>, <span style="background-color: #eef">debian: ("")</span>}</dd>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", <span style="color: red">2</span>)</span>, <span style="background-color: #eef">debian: ("")</span>}</dd>
<dd>_debian-revision_ defaults to 0 if not given</dd>

<dt><span style="background-color: #efe">2</span> < <span style="color: red;background-color: #fee">2</span> : <span style="background-color: #efe">1</span></dt>
<dd>{<span style="background-color: #fee">epoch: <span style="color: red">0</span>, upstream: ("", 2)</span>, <span style="background-color: #eef">debian: ("")</span>}</dd>
<dd>{<span style="background-color: #fee">epoch: <span style="color: red">2</span>, upstream: ("", 1)</span>, <span style="background-color: #eef">debian: ("")</span>}</dd>
<dd>_epoch_ defaults to 0 if not given</dd>

<dt><span style="background-color: #efe">1<span style="color: red">~rc</span>2</span> < <span style="background-color: #efe">1</span></dt>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", 1, "<span style="color: red">~rc</span>", 2)</span>, <span style="background-color: #eef">debian: ("")</span>}</dd>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", 1)</span>, <span style="background-color: #eef">debian: ("")</span>}</dd>
<dd>`~` sorts before the empty string</dd>

<dt><span style="background-color: #efe">1</span> < <span style="background-color: #efe">1<span style="color: red">.</span>2</span></dt>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", 1)</span>, <span style="background-color: #eef">debian: ("")</span>}</dd>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", 1, "<span style="color: red">.</span>", 2)</span>, <span style="background-color: #eef">debian: ("")</span>}</dd>
<dd>the empty string sorts before **everything** except `~`</dd>

<dt><span style="background-color: #efe">1</span> < <span style="background-color: #efe">1<span style="color: red">+gitABC</span>123DEF</span></dt>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", 1)</span>, <span style="background-color: #eef">debian: ("")</span>}</dd>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", 1, "<span style="color: red">+gitABC</span>", 123, "DEF")</span>, <span style="background-color: #eef">debian: ("")</span>}</dd>
<dd>Comparing hexadecimal numbers works - do you know why?</dd>

<dt><span style="background-color: #efe">1</span> < <span style="background-color: #efe">1</span> - <span style="color: red;background-color: #eef">2</span></dt>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", 1)</span>, <span style="background-color: #eef">debian: ("")</span>}</dd>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", 1)</span>, <span style="background-color: #eef">debian: ("", <span style="color: red">2</span>)</span>}</dd>

<dt><span style="background-color: #efe">1</span> - <span style="background-color: #eef">3</span> < <span style="background-color: #efe">1<span style="color: red">-</span>2</span> - <span style="background-color: #eef">3</span></dt>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", 1)</span>, <span style="background-color: #eef">debian: ("", 3)</span>}</dd>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", 1, "<span style="color: red">-</span>", 2)</span>, <span style="background-color: #eef">debian: ("", 3)</span>}</dd>
<dd>The _upstream-version_ changes because the pattern is greedy</dd>

<dt><span style="background-color: #efe">1</span> - <span style="background-color: #eef">2</span> > <span style="background-color: #efe">1</span> - <span style="background-color: #eef">2<span style="color: red">~bpo</span>9</span></dt>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", 1)</span>, <span style="background-color: #eef">debian: ("", 2)</span>}</dd>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", 1)</span>, <span style="background-color: #eef">debian: ("", 2, "<span style="color: red">~bpo</span>", 9)</span>}</dd>

<dt><span style="background-color: #efe">12.0.1<span style="color: red">-2</span></span> - <span style="background-color: #eef">dp1A~4.4.0.202011022025</span> > <span style="background-color: #efe">12.0.1</span> - <span style="background-color: #eef">3A~4.4.0.202108311259</span></dt>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", 12, ".", 0, ".", 1, "<span style="color: red">-</span>", 2)</span>, <span style="background-color: #eef">debian: ("dp", 1, "A~", 4, ".", 4, ".", 0, ".", 202011022025)</span>}</dd>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", 12, ".", 0, ".", 1)</span>, <span style="background-color: #eef">debian: ("", 3, "A~", 4, ".", 4, ".", 0, ".", 202108311259)</span>}</dd>
<dd>You have accidentally changes the _upstream-version_ by introducing an **addition hyphen before** the _debian-revision_</dd>

<dt><span style="background-color: #efe">1</span> - <span style="color: red;background-color: #eef">A</span> > <span style="background-color: #efe">1</span> - <span style="color: red;background-color: #eef">2</span></dt>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", 1)</span>, <span style="background-color: #eef">debian: ("<span style="color: red">A</span>")</span>}</dd>
<dd>{<span style="background-color: #fee">epoch: 0</span>, <span style="background-color: #efe">upstream: ("", 1)</span>, <span style="background-color: #eef">debian: ("", 2)</span>}</dd>
<dd>`ord("0")=48` would sort before `ord("A")=65`, but the former is a digit while the later is a non-digit. Therefore they are compared differently and `A` wins, as the comparison always starts with the non-digit-part.</dd>

</dl>

# Greedy vs. minimal

What most confuses people is that the _upstream-version_ seems to be _greedy_ in regard to _hyphens_:

> Only the **last** hyphen separates _upstream-version_ from _debian-revision_, so **all except the last** hyphen should be captured by _upstream-version_.
> Doesn't this make it _greedy_?

Actually no:

*   The greedy version `[0-9-]+-[0-9]+` would **mistakenly** capture everything as _upstream-version_ as it does **not stop** before the last hyphen.
*   The minimal version `[0-9-]+?-[0-9]+` leaves the last part to _debian-revision_ while capturing only the remaining prefix correctly as _upstream-version_.

{% include abbreviations.md %}
