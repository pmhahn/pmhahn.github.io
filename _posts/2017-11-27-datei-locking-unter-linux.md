---
title: 'Datei-Locking unter Linux'
date: '2017-11-27T12:04:14+01:00'
layout: post
categories: linux filesystem

Ich musste wieder einmal zu oft nachlesen, welche Varianten von Datei-Locking es unter Linux gibt und was die Stolperfallen sind.

| Variante | POSIX Lock | BSD Lock | Linux Open File Description Locks |
| -------- | ---------- | -------- | --------------------------------- |
| Range    | Byte       | File     | Byte                              |
| Owner    | Process    | File     | File                              |
| Issues   | - not `fork()` save - not thread save - released on first `close()` | - exclusive not `fork()` save - not NFS save - sometimes implemented through `fcntl()` | Linux 3.15+ |
| C-API    | [`fcntl(fd, F_{GETLK,SETLK,SETLKW}, struct flock *)`](https://www.gnu.org/software/libc/manual/html_node/File-Locks.html), `lockf(fd, F_{LOCK,TLOCK,ULOCK,TEST}, len)` | [`flock(fd, LOCK_{SH,EX,UN}`](https://www.freebsd.org/cgi/man.cgi?query=flock&sektion=2) | [`fcntl(fd, F_OFD_{GETLK,SETLK,SETLKW}, struct flock *)`](https://www.gnu.org/software/libc/manual/html_node/Open-File-Description-Locks.html) |

Literatur zum nach-/weiterlesen:

- [File locking in Linux](https://gavv.github.io/blog/file-locks/)
- [Advisory File Locking – My take on POSIX and BSD locks](https://loonytek.com/2015/01/15/advisory-file-locking-differences-between-posix-and-bsd-locks/)
- [On the Brokenness of File Locking](http://0pointer.de/blog/projects/locking.html)
- [File-private POSIX locks](https://lwn.net/Articles/586904/)
