---
layout: post
title: Prevent eCryptfs from asking for passphrase
date: 2020-08-27 16:20:00  +0200
categories: linux debian security
excerpt_separator: <!--more-->
---

For historical reasons I have been using [eCryptfs](https://www.ecryptfs.org/), a file system layer for encrypted files.
It got removed from [Debian Buster](https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=928956), but I'm still using it.

For transparent usage it installs its own PAM module:
When you log in your password can be used to automatically decrypt your files.
You can also use a different passphrase to improve security even more.

But this shows an annoying behavior, as you also get asked for that additional passphrase when you use `sudo` or other tools.

I (temporarily) fixed this by changing my `/etc/pam.d/common-auth` to use [pam_succeed_if](https://linux.die.net/man/8/pam_succeed_if) like this:

```
auth    [default=1 success=ignore]      pam_succeed_if.so service notin sudo:polkit-1
auth    optional        pam_ecryptfs.so unwrap
```

This skips the call to `pam_ecryptfs` if the service is either `sudo` or `PolicyKit-1`, which is used by the update service.

*[PAM]: Pluggable Authentication Modules
