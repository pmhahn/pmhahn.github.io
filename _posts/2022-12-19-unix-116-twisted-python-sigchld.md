---
title: 'UNIX 116: Twisted Python SIGCHLD'
date: '2022-12-19T16:52:27+01:00'
layout: post
categories: python
---

Univention Corporate Server (UCS) is a Debian GNU/Linux based enterprise operating system.
For automatic installation testing the Python package [vncdotool](https://github.com/sibson/vncdotool) is used to click through the installation process running inside a virtual machine using [QEMU](https://www.qemu.org/).
Most of the time this worked without problems but recently our tests were failing on a regular basis without any major change to our code base.
Any *expert* will already think of *timing issues* and at the end it actually will be.

I invite you to follow me on the twisted road deep down into the technical details of our test infrastructure called `vnc-automate`.

## Motivation

In 2016 we implemented a [framework for testing the graphical Debian installer and UMC Setup](https://forge.univention.org/bugzilla/show_bug.cgi?id=43218).
As a complete operating system provider we wanted to make sure that our users can use our latest ISO images to install UCS on their own hardware.
Previously those tests were done by hand.
While not itself complex they needed a lot of time, so they were not done on a regular basis.
We only performed them for new releases or when we changed things related to the installation process if we remembered to redo them.
By automating these steps we were able to detect breaking changes much earlier which then allowed us to fix them as early as possible.

Before that we already experimented with different solution:
The initial solution just compared the screen to screenshots taken from the previous run.
This was easy to implement but hard to maintain as each changed text required the screenshot to be re-done.

<!-- ![UCS 3.2 Installer](https://hutten.knut.univention.de/blog/wp-content/uploads/2022/12/language-300x225.png) -->
UCS-3 only had a text-based installer, where the graphical representation could be easily mapped back to the text.
That was done by comparing each fixed size block with glyphs from the chosen font.
That way we could search for text, which made things a lot easier.

<!-- ![UCS 4.0 Installer](https://hutten.knut.univention.de/blog/wp-content/uploads/2022/12/installer-language-300x225.png) -->
With UCS-4 we dropped our own *curses* based installer and [switched](https://forge.univention.org/bugzilla/show_bug.cgi?id=30547) to the [Debian-Installer](https://www.debian.org/devel/debian-installer/).
It provides both a textual and graphical variant, where the later is the default for our users.
As such that should be tested.
That made the previous approach for *text analysis* useless and we were forces to find a new way.

We decided to use [vncdotool](https://github.com/sibson/vncdotool) to interact with the virtual machine and to use the [Tesseract Open Source OCR Engine](https://github.com/tesseract-ocr/tesseract) to work with text instead of images.
We implemented a thin wrapper around this, which we call [vnc-automate](https://github.com/univention/vnc-automate).
Under the hood [Python twisted](https://twisted.org/) is used, which is <q>an event-driven networking engine written in Python</q>.

And there the problems start…

## Concurrent programming

*Twisted* was established 2001 and provides a framework for writing highly concurrent networking services.
Python gained native support for `async` only via [PEP-492](https://peps.python.org/pep-0492/) in 2015, so *Twisted* is way older.
*Asynchronous Input-Output* has the benefit over *multi-threaded code* , that you as a programmer can decide where to switch context instead of the operating systems scheduler doing it anytime, even when you program state is currently inconsistent.
`async` programming has become very popular in the recent years as it improves efficiency and gets rid of many concurrency issues known from multi-threading.
It is best used for IO-bound tasks where your process mostly waits for external events like network connections, database queries or even file-system activity.
It is not useful for CPU-bound work-loads as most implementations only use a single CPU core.
For this kind of work-load *multi-processing* or *multi-threading* are more appropriate.
But because of the [Python Global Interpreter Lock](https://realpython.com/python-gil/) *multi-threading* is not an option there and you have to use *multi-processing* if you want to run tasks on multiple CPU cores in parallel.

## The Python twisted reactor

The core of *Twisted* is the [reactor](https://docs.twisted.org/en/stable/core/howto/reactor-basics.html), which *reacts* to events and then runs the appropriate code to handle these events:
Accept a new network connection when a connection request arrives, send more data to some other process which is now ready to receive more data, handle a timeout of another party did non react in time, and so on.

`vncdotool` uses *Twisted* under the hood to handle all its network traffic:
request a screen update and handle the many chunks to update the local copy of the screen, send keyboard and mouse button events to the server, handle screen size changes from the server, ….
By using *Twisted* `vncdotool` can handle many VNC connections at the same time, which might be useful for a classroom situation, where you want to monitor multiple student PCs at the same time and display them all on the PC of the teacher.
While we could probably find a solution for our tool to **not** use the *reactor*, the decision to always use it was done by *vncdotool* and we have to work with that.
There are two options:
Either you buy fully into the asynchronous idea and write an asynchronous program yourself, or you use the [synchronous API](https://vncdotool.readthedocs.io/en/latest/library.html):
In that case you write a traditional synchronous program and let `vncdotool` run its own reactor in a background thread to make it work itself.
The later is what was chosen.

In addition to using VNC to get the screen and to interact with the virtual machine `vnc-automate` also has to handle the *optical character recognition* by calling `tesseract`.
Back then this was done in the **asynchronous-part** of the application, which allowed us to invoke multiple such processes at the same time.
Looking at the code you will find a [strange comment](https://github.com/univention/vnc-automate/blob/master/vncautomate/ocr.py#L100-L103):
```python
ef outConnectionLost(self):
 # FIXME: It's unclear what happens here, but without the sleep
 # processEnded() won't be called.
 time.sleep(0.5)
```

So already back then something was not right…

## Fast forward: Python 3

As <q>legacy Python 2</q> has reached its [end-of-life](https://www.python.org/doc/sunset-python-2/) in 2020 `vnc-automate` needs to be migrated to <q>modern Python 3</q>.
While working on that issue something strange happened:
the first screen was OCR'ed fine, but the process just got stuck on the next screen or later on.
None of my runs ever completed.
Debugging *Twisted* is not easy as you get many context switches:
you do not get a full stack trace, but only get to see what happens while handling a single [Defered](https://docs.twisted.org/en/stable/core/howto/defer-intro.html).
At one point I even had to revert to *print-statement-debugging* to rule out any bug in the [PyCharm](https://www.jetbrains.com/pycharm/) debugger, but for a long time I was stuck finding the culprit until I noticed the following phenomena:

## Zombies

The [code](https://github.com/univention/vnc-automate/blob/master/vncautomate/ocr.py#L478-L482) would start multiple `tesseract` processes in parallel.
For inter-process-communication *Twisted* sets up each process to use pipes for STDIN, STDOUT and STDERR, so `vnc-automate` can send data to `tesseract` and is able to receive and log its output.
As each process terminates [results are gathered](https://github.com/univention/vnc-automate/blob/master/vncautomate/ocr.py#L500) at the end.
That last step never concluded and the `vnc-automate` was waiting there for some processes to terminate.
Looking at the process tree showed this picture:
```
PID TTY STAT START TIME COMMAND
562170 pts/11 Sl+ 13:40 0:07 python3 ./vnc-install-ucs.py –vnc localhost:1 –language deu –role master –fqdn mytest.ucs
562204 pts/11 Z+  13:40 0:00 \_ [tesseract]
```

Here most children processes had already terminated, but one of them was left behind as a [Zombie process](https://en.wikipedia.org/wiki/Zombie_process):
The child process itself has already exited, but the parent process did not yet collect its exit status.
This should not happen with *Twisted* as the *reactor* is supposed to just handle that for us:
Close the pipes used for inter-process-communication between the parent and child process and collect the exit code.
But this is somehow tricky as a dying child will trigger 4 events in this constellation:

- the dying child will close its reading end of the `STDIN` pipe, so the writing end of that pipe will become *writable*.
The `reactor` closes its writable end as any subsequent `write()` to the closed pipe would otherwise trigger a `SIGPIPE`, which normally would terminate the *producing* process.
- the dying child will close its writing end of the `STDOUT` pipe, so the reading end of that pipe will become *readable* and return *end-of-file* (EOF)
- similar for the `STDERR` pipe
- After the child died the kernel will send the signal `SIGCHLD` to its parent process.

The order in which the Linux kernel and the Twisted `reactor` handles them is arbitrary (at least I found no specification for this;
if you know some please contact me.)
Looking at the [Linux kernel code for `do_exit()`](https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/kernel/exit.c?h=v6.0.14#n790) we see that `exit_files()` is called before `exit_notify()`, which sends the `SIGCHLD` to the parent process.
So probably mostly you well see the events orderd as above.
Keep that in mind for later.

Because of that unspecified ordering *Twisted* [ProcessProtocol](https://docs.twisted.org/en/stable/api/twisted.internet.interfaces.IProcessProtocol.html) provides two callback methods [processExited](https://docs.twisted.org/en/stable/api/twisted.internet.interfaces.IProcessProtocol.html#processExited) and [processEnded](https://docs.twisted.org/en/stable/api/twisted.internet.interfaces.IProcessProtocol.html#processEnded).
The first is called on reception of `SIGCHLD`, while the second is supposed to be called when all (4) events have been processed.

## SIGCHLD

A UNIX process gets informed by the Kernel whenever a child-process dies:
The parent process gets a signal [SIGCHLD](https://man7.org/linux/man-pages/man7/signal.7.html) (or `SIGCLD`), which it then can use to react on the demise.
But similar to interrupts interrupting the kernel at whatever it is doing, signals interrupts the process at whatever it is doing:
if the process is currently executing a system call that call returns early returning an error code.
The process then must check [errno](https://man7.org/linux/man-pages/man3/errno.3.html) and will find it set to `EINTR`, which indicates an <q>Interrupted system call</q>.
The process is then supposed to re-execute the last system call by hand.
Because of that most system calls must be put inside a look like
```c
do {
 rc = syscall(…)
} while (rc == -1 &&
errno == EINTR);
```

This adds a lot of boilerplate code to each system call.
BSD introduced a second behavior called `SA_RESTART` which automatically re-executes the interrupted system call.
This simplifies the application code and also handles [other issues](man:restart_syscall(2)), which is why the BSD semantics is mostly preferred.

The *Twisted reactor* basically is a loop using [`select()`](man:select(2)), [`poll()`](man:poll(2)) or [`epoll()`](man:epoll(7)) to wait for events:
All these system calls get a list of *file descriptors* which are monitored:
if any one of them becomes readable (because someone on the other end send data) or writable (the other end read some data so there is now room again to send more data) or are closed (`EOF` becomes readable), that file descriptor is mapped to an event which then triggers the registered Python code.
There also is a parameter `timeout` which gets used to timeout the wait just before the next event is scheduled.

[signal(7)](man:signal(7)) lists `select()` and the other ones as exceptions:
They are always interrupted, both with the original UNIX semantics and with `SA_RESTART`:
This is good as `SIGCHLD` interrupts that `select()` call and the reactor would be able to check for any dead child process using `waitpid(pid, WNOHANG)`.
But the `reactor` does not do this by itself, but needs another trick.

## Talking to myself

To translate `SIGCHLD` into something `select()` can wait on, the signal needs to be mapped into an event on a file-descriptor.
This is called [the self-pipe trick](http://cr.yp.to/docs/selfpipe.html):
The process opens a [`pipe()`](man:pipe(2)) and keeps both of its end to itself.
The *read end* gets passed to `select()` while for the *write end* a signal handler is registered for `SIGCHLD`, which writes a single character to it.
This sound elegant but is full of gotchas:
A pipe only has a limit capacity — traditionally it was 8-16 KiB, but modern Linux is mostly only limited by available memory.
Despite that you should change the pipe into **non-blocking mode** as otherwise your signal handler could block while writing to the pipe:
as there is no other process to ever read from the reading end, your process would be stuck forever.
You also have to be very careful to handle all cases of concurrency correctly:
If multiple processes die at the same time multiple `SIGCHLD` will be sent, but may be aggregated.
Therefore you should always check for multiple dead processes and should also handle the case, where you get triggered, but already handled the child in the previous round.

That requirement to handle file-descriptor activity and signals at the same time is very common:
The Linux kernel even added the system-call [`signalfd()`](man:signalfd(2)) for that a long time ago in version 2.6.22.
(Linux Kernel 5.3 even has [`pidfd_open()`](man:pidfd_open(2)), which directly provides a file descriptor to monitor child processes, but neither [glibc](https://www.gnu.org/software/libc/) nor Python do provide this new system call.)
Python has the built-in function [`signal.set_wakeup_fd()`](https://docs.python.org/3/library/signal.html#signal.set_wakeup_fd) to set this up as part of the standard library.
And [Python twisted](https://docs.twistedmatrix.com/en/stable/api/twisted.internet._signals.html) does use it.

Or not.

## And here it comes crashing down

You remember that `vncdotool` offers two APIs, a asynchronous one and the synchronous one?
And that the later on is starting the `reactor` in a background thread?
As you have learned from above signals are already tricky enough and there are many more pitfalls.
They come from a time when multi-threading was still far away and adding threads to signals opened up a new can of worms:
Which thread of a process should get interrupted to handle the signal?
Should the other threads also get notified?
And so on and so on.
Because of this the **main thread** — the one that the process started with initially — by defaults handles all signals in Python.
You can change this on modern Linux/Python, but by default the main thread is special.
Some calls related to signal handling even **must only be called from the main** thread.
Quoting from the Python documentation for [`signal.set_wakeup_fd()`](https://docs.python.org/3/library/signal.html#signal.set_wakeup_fd) again:

> When threads are enabled, this function can only be called from the main thread of the main interpreter;
> attempting to call it from other threads will cause a ValueError exception to be raised.

As `vncdotool` runs the `reactor` in a background thread it has to [disable the use](https://github.com/sibson/vncdotool/blob/main/vncdotool/api.py#L139-L140) of `set_wakup_fd()`, as otherwise the mentioned `ValueError` exception would be raised:

```python
_THREAD = threading.Thread(target=reactor.run, name="Twisted",
 kwargs={'installSignalHandlers': False})
```

So `vncdotool` runs a variante of the `reactor`, which is using none of the above:
Neither the *Self-pipe-trick* not `signalfd()` nor `pidfd_open()`.
The only events the `reactor` sees are those related to the inter-process-pipes being closed, but not the actual `SIGCHLD` event.

So with `installSignalHandlers=False` this is what happens:

1. STDIN is closed by us just after the process is started as we do not need to feed data into the sub-process.
2. `tesseract` runs and writes to STDOUT and STDERR until it terminates successfully.
3. STDOUT and STDERR are closed. Each event gets passed to the protocol as [`childConnectionLost`](https://github.com/twisted/twisted/blob/trunk/src/twisted/internet/protocol.py#L644).
4. After each closed pipe [`Process.childConnectionLost`](https://github.com/twisted/twisted/blob/trunk/src/twisted/internet/process.py#L915) calls [`maybeCallProcessEnded`](https://github.com/twisted/twisted/blob/trunk/src/twisted/internet/process.py#L926) to check, is all pipes are closed. Only when they are all closed [`reapProcess()`](https://github.com/twisted/twisted/blob/trunk/src/twisted/internet/process.py#L280) is called.
5. That one uses `waitpid()` to check the status of the child process. If the child-process has exited [`processEnded()`](https://github.com/twisted/twisted/blob/trunk/src/twisted/internet/process.py#L308) is called.

Which brings us back the that [strange comment](https://github.com/univention/vnc-automate/blob/master/vncautomate/ocr.py#L101-L103) from the beginning:
```python
# FIXME: It's unclear what happens here, but without the sleep processEnded() won't be called.
```

The `sleep(0.5)` just adds enough delay to the parent `vnc-automate` process running the `reactor`, so that the dying child process can fully finish in parallel and send its `SIGCHLD` to the parent process.
When the `reactor` in the parent process then continues with `outConnectionLost` called from `childConnectionLost`, the exit status of the now dead child process is then already available and `processEnded()` gets called as part of the *file descriptor activity*, not because of the (never hanled) `SIGCHLD` event.

## Conclusion

Signal handling in UNIX is already tricky, but mixing it with multi-threading and asynchronous techniques just may be too much for it to work.
Debugging these kind of issues becomes double hard as — as soon as you add debugging — you change the timing:
adding a `print()` here or a `breakpoint()` there inevitable adds enough delays that then you're no longer able to observe the underlying issue.

Timing issues are always a pain:
seeing any `sleep()` call make me nervous as these kind of *fixes* fail to work sooner or later.
Finding the underlying problem often requires a deep understanding of all the layers below, which is not easy and takes time.
But if you gained that inside, finding a better solution is then possible.
For me this is one of the major benefits of Open-Source-Software as with it you at least *can look at the code*, where with Closed-Source-Software you're stuck and at the mercy of the manufacturer.

For `vnc-automate` I will move the code calling `tesseract` move out of the asynchronous into the synchronous part, so that the reactor is only used to handle the VNC protocol using the synchronous API.

Hopefully you enjoyed the ride with me and have learned something new for you next debugging session.

{% include abbreviations.md %}
