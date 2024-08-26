---
layout: post
title: "UNIX domain socket"
date: 2019-08-28 09:00:00  +0200
categories: linux
excerpt_separator: <!--more-->
---

Q: How do you find the process listening on an UNIX domain socket?

<!--more-->

# Intro

UNIX domain sockets - like TCP sockets - provide a bi-directional for communication between processes.
But in contrast to TCP sockets, they only exist locally on the host.
As such they cannot be used for inter-host-communication.

UNIX domain sockets do not use port numbers to distinguish between multiple instances.
Traditionally they use paths and are represented in the file system as *socket* Inodes.
But there also exists a second variant, which is described in [below](#abstract-sockets).

There also is a limit on the maximum path length, with is `UNIX_PATH_MAX = 108` characters, so be careful to not use deeply nested directories.

Some operating systems other then Linux do not check the file permissions on the socket Inode itself.
Therefor it is good practice for portable applications to put the socket in a separate directory with only read- and search-permission for the desired user or group.

More details are described in the manual page <man:unix(7)>, which also includes an example C program.
Python also provides support and includes server code in [socketserver](https://docs.python.org/3/library/socketserver.html).

# The problem

If we starts two processes listening on the same path, which one will get new connections?

```console
$ nc -l -N -U /tmp/socket &
$ ls -i /tmp/socket
1569897 /tmp/socket
^^^^^^^
$ nc -l -N -U /tmp/socket &
$ ls -i /tmp/socket
1569946 /tmp/socket
^^^^^^^
```

As you see, both listening processes create a *new* UNIX socket Inode in the file system.
Opening a connection on that path will thus reach only one process: the *latest*:

```console
$ nc -N -U /tmp/socket </dev/null
[2]+  Fertig                  nc -l -N -U /tmp/socket
$ nc -N -U /tmp/socket </dev/null
nc: unix connect failed: Connection refused
```

So given the *path* we want to find the *process* serving that *Inode*!

# The duplicate issue

Multiple processes serving the same socket often happens by accident as `bind()` fails if the socket still exists.
If the server process crashes or does not cleanup the path itself, the server will fail to start next time.
The following Python program shows this nicely:

```python
import socket
s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
s.bind('/tmp/socket')
```

```
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
  File "/usr/lib/python2.7/socket.py", line 228, in meth
    return getattr(self._sock,name)(*args)
socket.error: [Errno 98] Address already in use
```

Therefore many services unconditionally unlink the socket file before starting.
If the old service is still running, its socket will no longer be reachable.

Also be aware that `close()`ing the socket will not remove the socket path, so your daemon should `unlink()` it itself when shutting down.

# The naive try

Do you know `fuser` and `lsof`?
You should!
They are useful tools for finding processes having opened files and sockets:

```console
$ fuser /tmp/socket
/tmp/socket:         12550 12554
$ lsof /tmp/socket
COMMAND   PID  USER   FD   TYPE             DEVICE SIZE/OFF     NODE NAME
nc      12550 phahn    3u  unix 0x000000005149f1ab      0t0 13381347 /tmp/socket type=STREAM
nc      12554 phahn    3u  unix 0x00000000ac502341      0t0 13383220 /tmp/socket type=STREAM
                                                            ^^^^^^^^
```

They both find *both* processes, but we know that only the latest will get all the requests.
Even worse they might provide no output at all:

```console
$ cd /tmp
$ fuser ./socket
/tmp/socket:         12550 12554
$ lsof ./socket
```

Yes, the output of `lsof` remains empty!
It also does if your path contains *symbolic links* and you are using the *wrong* path to the socket Inode.

So the tools are less ideal for UNIX sockets as the problem there is more difficult:

* For a file you have the *file system ID* and the *Inode* within that file system, which uniquely identifies the file.
  The tools just have to walk `/proc/$pid/fd/$fd` and compare this to the 2-tuple of the requested files.

* Same for UDP and TCP sockets, where the kernel guarantees that only one process per host can uniquely use that socket (`fork()`s ignored).

But for UNIX domain sockets we already have seen that multiple processes can open the same path for serving, each one gets a new UNIX socket Inode and thus only the latest process gets all new requests.
All previous serving processes remain until they terminate themselves or are killed by someone.

Linux provides some information on UNIX domain sockets in its `/proc` file system in `/proc/net/unix`:

```console
$ grep /tmp/socket /proc/net/unix
# Num               RefCount Protocol Flags    Type St Inode    Path
# 000000005149f1ab: 00000002 00000000 00010000 0001 01 13381347 /tmp/socket
# 00000000ac502341: 00000002 00000000 00010000 0001 01 13383220 /tmp/socket
                                                       ^^^^^^^^
```

Both `fuser` and `lsof` use that information to find the processes, which opened UNIX sockets.
But;

* `Path` matches exactly the string, which was used by the process when it called `bind()`.
  If the sockets gets moved or renamed, both `fuser` and `lsof` can no longer match the path to a process.
  `lsof` even will fail, when the path is relative or is different because it contains symbolic links.

  You might assume, that the socket Inode in the file system is not relevant, but it is.
  If you manually rename the socket afterwards, a program can still connect your daemons on the renamed path:

    ```console
$ mv /tmp/socket /tmp/socket.old
$ nc -N -U /tmp/socket.old </dev/null
```

* The listed `Inode` in is different from the Inode in the file system:
  To handle sockets like files the Linux kernel implements a virtual file system called `sockfs`:

    ```console
$ grep sock /proc/filesystems
nodev   sockf
```

  For each opened socket it contains an Inode, which then the processes reference:

    ```console
$ readlink /proc/12554/fd/3 /proc/12550/fd/3
socket:[13383220]
socket:[13381347]
        ^^^^^^^^
```

  The numbers in the square brackets 'socket:[...]` match the Inode numbers from `/proc/net/unix'.

But we are still missing the link from the Inode in the file system path to the socket Inode of the process.

The question is quiet popular and on [StackExchange](https://unix.stackexchange.com/questions/16300/whos-got-the-other-end-of-this-unix-socketpair) you will find interesting solutions like using `gdb /proc/kcore`.
The `Num` column really is a kernel address, which can be linked to a [struct unix_sock](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/net/af_unix.h?id=master#n53), which then can be used to find the process.

# Using `ss`

The Linux kernel implements the <man:sock_diag(7)> extension since 4.2, which provides additional information to diagnose socket issues.
Using `ss` from [iproute2](https://wiki.linuxfoundation.org/networking/iproute2) starting with version [v4.19.0~55](https://git.kernel.org/pub/scm/network/iproute2/iproute2.git/commit/?id=0bab7630e38863d3d2a5ddaeabf8745c4258a1a9) does show the VFS information:

```console
$ ss --processes --unix --all --extended 'sport = /tmp/socket'
Netid  State   Recv-Q  Send-Q  Local Address:Port      Peer Address:Port
u_str  LISTEN  0       5         /tmp/socket 13381347             * 0     users:(("nc",pid=12550,fd=3)) <-> ino:1569897 dev:0/65025 peers:
u_str  LISTEN  0       5         /tmp/socket 13383220             * 0     users:(("nc",pid=12554,fd=3)) <-> ino:1569946 dev:0/65025 peers:
                                             ^^^^^^^^                                                           ^^^^^^^
```

This both lists the socket Inode as *Port* and the path Inode in the last column as *ino:*.
`dev` corresponds to the device number given by `stat`:

```console
$ stat -c 'ino:%i dev:0/%d' /tmp/socket
ino:1569946 dev:0/65025
```

# Finally

So finally we have a solution:

1. `stat` the socket to get the device ID and Inode
2. Use `ss --processes --unix --all --extened` to look-up that tuple and match it to the process ID.

# More details

There are some more *nice-to-know* details about *UNIX domain sockets*, which are mostly irrelevant for this problem so far.
But I want to mention them anyway for reference:

## Abstract sockets
There exist a second class called *abstract sockets*:
* their path name starts with the `NUL` characters, making their path length 0.
* then they can use the remaining 107 characters to define a unique identifier, which other programs can use to connect.
* they are not represented in the file system.

## Credential passing
As the communicating processes are both local to the host, the listening process can get additional information from the sender.
This includes the *process*, *user* and *group* IDs (PID, UID, GID).
See `SCM_CREDENTIALS` in the manual page.

## File descriptor passing
UNIX domain sockets can be used to pass file descriptors between processes.
This can be used to have a privileged process, which opens a file on request of another process.
Usually it performs additional checks before it passes its opened file on to the requesting process.
See `SCM_RIGHTS` in the manual page.
Python only supports this natively since Python-3.3.

*[VFS]: Virtual File System
*[PID]: Process Identifier
*[UID]: User Identifier
*[GID]: Group Identifier
*[ID]: Identifier
*[TCP]: Transmission Control Protocol
*[UDP]: User Datagram Protocol
