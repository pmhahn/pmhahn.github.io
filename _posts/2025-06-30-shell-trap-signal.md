---
title: 'shell `trap` signal'
date: '2025-06-30T07:47:00+02:00'
layout: post
categories: shell
excerpt_separator: <!--more-->
---

What's wrong with signal handling like this:
```sh
#!/bin/sh
trap 'echo Cleanup…' EXIT HUP INT TERM
...
```

<!--more-->

## Exit and signals

Before we begin:
Actually _exit codes_ are **mutual exclusive** to _signal statuses_:
A process may either exit normally using `exit` or terminate via a signal.

If you read [man:bash](man:bash(1)) you will read this:
> The return value of a simple command is its exit status, or 128+n if the command is terminated by signal n.

That might give you the idea, that they are the same, but that is only a (broken) shell convention to map _signal statuses_ to _exit codes_.
Reading [man:exit](man:exit(2)) you see this:
> The value status & 0xFF is returned to the parent process as the process's exit status,

So there are 256 exit codes from 0 to 255, which a process can use to exit.

The parent process then uses [waitpid()](man:waitpid(2)) to wait for the childs _state change_:
> That may be the process exited by calling `exit()` itself or caught a `signal()`, which might have `kill()`ed the process or just suspended it.

You then have to first use `WIFEXITED()` or `WIFSIGNALED()` to check, if the child exited normally via `exit()` or caught a `signal()`.
Only after that you should either use `WEXITSTATUS()` to extract the byte containing the _exit code_ or use `WTERMSIG()` to extract the _signal number_.

In a shell script you do not have access to these low-level C functions, but only get the mangled exit status.
You cannot distinguish is the called process did `exit(130)` itself or was terminated by the user pressing _Ctrl-C_ so send `SIGINT` to it.

## Signals and EXIT trap

Here's a short overview of commonly used signals and traps.

| signal  | number | trigger | when                |
| ------- | -----: | ------- | ------------------- |
| EXIT    | "0"    | `exit`  | shell process exits |
| SIGHUP  | 1      |         | login TTY closed    |
| SIGINT  | 2      | Ctrl-C  | user aborts process |
| SIGQUIT | 3      | Ctrl-\  | user aborts process |
| SIGTERM | 15     |         | `kill $PID`         |

Please not that shells misuse signal `0` here:
By default there is not signal numbered `0`.
Actually it is a no-operation and can be used to check, if _process A can send signals to process B_ or if _process B is still alive_.
`bash` and other shells re-use that number to give their `EXIT` handler a number, which is supposed to be called on _any exit from shell_.
But that behaviour is very implementation dependant as you will see later on.

## Implementation specific handling of EXIT

Let's try this with the more informative shell script `trap.sh`:
```sh
#!/bin/bash
cleanup () {
    local rv=$? sig=${1:-0}
    echo "Process $$ received signal $sig after rv=$rv"
    case "$sig" in
    0|'') exit "$rv";;
    *) trap - "$sig"; kill "-$sig" "$$";;
    esac
}
trap 'cleanup  0' EXIT
trap 'cleanup  1'  1  # SIGHUP
trap 'cleanup  2'  2  # SIGINT
trap 'cleanup  3'  3  # SIGQUIT
trap 'cleanup 15' 15  # SIGTERM

[ -n "${1:-}" ] && kill "-$1" "$$"
```

```console
$ bash ./trap.sh 0  # EXIT
Process 499218 received signal 0 after rv=0
$ bash ./trap.sh 1  # SIGHUP
Process 499237 received signal 1 after rv=0
Process 499237 received signal 0 after rv=0
Hangup
$ bash ./trap.sh 2  # SIGINT
Process 499256 received signal 2 after rv=0
Process 499256 received signal 0 after rv=0

$ bash ./trap.sh 3  # SIGQUIT
Process 499275 received signal 3 after rv=0
Process 499275 received signal 0 after rv=0
$ bash ./trap.sh 15  # SIGTERM
Process 499294 received signal 15 after rv=0
Process 499294 received signal 0 after rv=0
Terminated
```

As you can see [`bash`](https://www.gnu.org/software/bash/) **always** calls the trap handler for `EXIT`!
Let's repeat this with [`dash`](http://gondor.apana.org.au/~herbert/dash/):
```console
$ dash ./trap.sh 0
Process 502873 received signal 0 after rv=1
$ dash trap.sh 1
Process 501892 received signal 1 after rv=0
Hangup
$ dash trap.sh 2
Process 501912 received signal 2 after rv=0

$ dash trap.sh 3
Process 501929 received signal 3 after rv=0
Verlassen (Speicherabzug geschrieben)
$ dash trap.sh 15
Process 501971 received signal 15 after rv=0
Terminated
```
And once more with [`busybox`](https://busybox.net/):
```console
$ busybox sh trap.sh 0
Process 502338 received signal 0 after rv=0
$ busybox sh trap.sh 1
Process 502366 received signal 1 after rv=0
Hangup
$ busybox sh trap.sh 2
Process 502402 received signal 2 after rv=0

$ busybox sh trap.sh 3
Process 502439 received signal 3 after rv=0
Process 502439 received signal 0 after rv=0
$ busybox sh trap.sh 15
Process 502269 received signal 15 after rv=0
Terminated
```

There `EXIT` is _almost_ never called, except by `busybox` on `SIGQUIT`.

That is why **portable** shell scripts setup `trap` not only for `EXIT`, but also for other `SIG`nals.

But if you do that, please make sure to do it right:
1. Reset the `trap` handler to its default.
2. Afterwards kill the process by re-sending the received signal to the process again.

## Why proper trap handling is important

Viacheslav Biriukov wrote a great blog post about [Process groups, jobs and sessions](https://biriukov.dev/docs/fd-pipe-session-terminal/3-process-groups-jobs-and-sessions/) explaining why proper exiting is important.
A program might setup a signal handler for `SIGINT` to prevent the program from just terminating, which might loose important data.
It might ask the user if terminating is okay or if the data should be saved first before quitting.
A surrounding shell script must then decide, if this is an _abnormal exit_ and should terminate itself **afterwards**, or should continue normally.
The UNIX convention is to transfer that detail via _exit codes_ and _signal statuses_.
So be careful and do it right if your shell script starts  using `trap`.

## Conclusion

1. Use `bash` as it has consistent handling of `trap EXIT`.
2. If you want to or must use other shells: Do not use the same `cleanup` trap of `EXIT` and other signals.
3. If you trap signals, make sure to reset the handler and to re-raise the signal to properly propagate them.
