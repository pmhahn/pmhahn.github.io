---
title: 'Linux and the Windows NT file system'
date: '2026-02-17T16:01:00+02:00'
layout: post
categories: linux filesystem
excerpt_separator: <!--more-->
---

The _Windows New Technology File System_ (NTFS) has a long history with Linux:

| Driver          | Type   | Based on   | Kernel | Period    | Read-write |
|-----------------|--------|------------|--------|-----------|------------|
| Original        | Kernel | Scratch    | 2.1.74 | 1995-2001 | Read-only  |
| [Linux-NTFS][1] | Kernel | Scratch    | 2.5.11 | 2002-2024 | Read-only  |
| [Captive][2]    | FUSE   | `ntfs.sys` |        | 2003-2006 | Read-write |
| [NTFS-3G][3]    | FUSE   | Linux-NTFS | 3.18   | 2006-     | Read-write |
| [NTFS3][4]      | Kernel | Paragon    | 5.15   | 2021-2026 | Read-write |
| [NTFS Plus][5]  | Kernel | Linux-NTFS | 7.0?   | 2026-     | Read-write |
| AVM NTFS        | Kernel | NTFS-3G    |        | 2012-     | Read-write |

[1]: https://flatcap.github.io/linux-ntfs/misc.html
[2]: https://en.wikipedia.org/wiki/Captive_NTFS
[3]: https://en.wikipedia.org/wiki/NTFS-3G
[4]: https://www.kernel.org/doc/html/latest/filesystems/ntfs3.html
[5]: https://git.kernel.org/pub/scm/linux/kernel/git/linkinjeon/ntfs.git/log/?h=ntfs-next&ref=itsfoss.com

<!--more-->

1. The _Original_ implementation was from Martin von Löwis.
2. Anton Altaparmakov created the 2nd implementation _Linux-NTFS_ from scratch, which replaced the original implementation.
3. _Captive_ was the first user-space based implementation, which used the original Windows driver `ntfs.sys` from Microsoft and run it under Wine.
4. _NTFS-3G_ also runs in user-space and uses FUSE to talk to the kernel.
5. Paragaon donated an open-source version of if proprietary _NTFS3_ to the Linux kernel. It was the first read-write implementation in the kernel, but is less documented.
6. _NTFS Plus_ is based on the older _Linux-NTFS_ implementation, adds read-write support and updates the implementation to use modern Linux APIs. It is scheduled to replace _NTFS3_ again.
7. AVM – now FRITZ! Technology – ported the _NTFS-3G_ to kernel space and is used in FritzOS only.

```mermaid
gantt
    title NTFS
    dateFormat  YYYY-MM-DD
    axisFormat %Y
    Original    : 1997-01-01, 2002-04-01
    Linux-NTFS  : 2002-04-01, 2024-01-01
    Captive     : 2003-01-01, 2006-01-01
    NTFS-3G     : 2006-01-01, 2030-01-01
    NTFS3       : 2021-11-01, 2026-01-01
    NTFS+       : 2026-03-01, 2030-01-01
    ANTFS       : 2012-01-01, 2030-01-01
    2.0.0       : vert, 1996-06-09, 1m
    2.2.0       : vert, 1999-01-26, 1m
    2.4.0       : vert, 2001-01-04, 1m
    2.6.0       : vert, 2003-12-18, 1m
    2.6.16      : vert, 2006-03-20, 1m
    2.6.27      : vert, 2008-10-09, 1m
    3.0         : vert, 2011-07-21, 1m
    3.8         : vert, 2013-02-18, 1m
    4.4         : vert, 2016-01-10, 1m
    4.19        : vert, 2018-10-22, 1m
    5.10        : vert, 2020-12-13, 1m
    5.15        : vert, 2021-10-31, 1m
    6.1         : vert, 2022-12-11, 1m
    6.6         : vert, 2023-10-29, 1m
    6.12        : vert, 2024-11-17, 1m
```

## Links
- Wikipedia: [Linux Kernel version history](https://en.wikipedia.org/wiki/Linux_kernel_version_history)

<!-- <https://www.cyberark.com/resources/threat-research-blog/the-linux-kernel-and-the-cursed-driver -->
