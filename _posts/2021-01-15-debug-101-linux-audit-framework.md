---
title: 'Debug 101: Linux audit framework'
date: '2021-01-15T18:28:11+01:00'
layout: post
categories: linux
---

In the [UMC-server vs. python-notifier pullcord](https://forge.univention.org/bugzilla/show_bug.cgi?id=52518) we had the situation, that an **unknown** process kept killing **other** processes from time to time and we had **no idea, which process** it was.

We tried several things:

- Add code to `/etc/init.d/` scripts to log events.
- Add code to the `SIGTERM` signal handler of UMC-server and other services to log its invocation.
- Run `ps fax` on a regular basis to find the culprit by "accident".

This never showed a process forking a `kill -SIGTERM $PID`, so we needed a bigger hammer.

# strace

`strace` can be used to trace the system calls a process is doing.
(System calls are the "doors" between user-space and kernel-space and do the transition from unprivileged user-space code to privileged kernel code.)

- Using `strace "$cmd"` traces the system calls done by command `$cmd`.
- If you also want to (recursively) trance its child processes you have to use the `-f` argument.
  You also need this if the process is multi-threaded.
- Using `-o "$file"` the output gets written to the named file.
  For multiple processes you can give `-f` twice, so each process gets its own file (with suffix `.$PID`);
  otherwise all system calls of all processes are collected in only that single file.
- Using `-p "$PID"` you can attach to an already running process.
  Only that single process is traced, not its already running child processes.
  But you can use `-f` again to trace newly created child processed.
- You can use `-e t=process` to limit tracing to system calls from the "process" category.
  Other categories like "file" exists as well.
- Instead of categories you also can name individual system calls or exclude them, e.g. `-e '!futex'`.
- By default only the first 32 characters of strings are logged.
  Use `-s 256` to increase that.

# audit

The Linux kernel includes an [audit](https://github.com/linux-audit) framework, which you can think of as **`strace` on speed**.
The [Linux Audit framework](https://pmhahn.github.io/audit/) can do a lot more than system call tracing:
It was designed for CAPP-compliant auditing.

- Log any failed or successful access to a file like `/etc/shadow`.
- Intrusion Prevention System can use it to block processes from performing malicious actions.
- `systemd` logs the start and stop of services through it.
- PAM & Co. log important events when users log in or log out or change their permissions.
  This even allows to keep track of users using `sudo` or `su`.
- SELinux and AppArmor report and log violations via Audit.

First you have to install Debian package `auditd`, which for UCS is in "unmaintained".
```bash
ucr set repository/online/unmaintained=yes
apt-get -qq update
apt-get -q –assume-yes install auditd
```

For the pullcord case we needed to find the process doing a `sys_kill` with `a1=SIGTERM`.
This is done by adding the following rule to file `/etc/audit/rules.d/bug52518.rules`:

```
-a always,exit -F arch=b64 -S kill -F a1=15 -k Bug52518
```

This tells the Audit system to **always** emit an event, when the architecture **b64** system call **kill** is used with argument `SIGTERM` (**a1=15**).
The event is emitted on **exit** (after the Linux kernel already has executed its code) and should be tagged with key `Bug52518`.

In addition to that we also want to track processes doing an `sys_exit` or `sys_execve`, so we append a second rule:

```
-a always,exit -F arch=b64 -S exit -S execve -k Bug52518 
```

Here two system calls are used in the same rule, which is more efficient then adding individual rules.
(Performance easily becomes a problem as each additional rule slows down **all** system calls, so the complete system with all of its processes and threads.
Anecdote from customer 26:
too many rules led to to an release updates taking many hours instead of minutes.)

The individual audit rule snippets must be concatenated to a single file `/etc/audit/audit.rules` and loaded into the Linux kernel, which is done by running `augenrules --load`.

## auditd vs journald

Afterwards the audit events are both logged into `/var/log/audit/audit.log` and to "systemd journal".
To prevent the events being logged twice by `journald` and `auditd`, disable the first by running `systemctl mask systemd-journald-audit.socket`.

Most log events consists of multiple lines, which are linked via their common **Audit ID**.
The events are logged as **RAW** events, which makes them hard to read.
For example the time stamp is in *seconds since the UNIX epoch* and process titles are printed hex-encoded:

```
type=SYSCALL msg=audit(1610524812.243:7148742): arch=c000003e syscall=62 success=yes exit=0 a0=1312 a1=f a2=1 a3=20 items=0 ppid=1 pid=5965 auid=4294967295 uid=0 gid=0 euid=0 suid=0 fsuid=0 egid=0 sgid=0 fsgid=0 tty=(none) ses=4294967295 comm="univention-mana" exe="/usr/
type=OBJ_PID msg=audit(1610524812.243:7148742): opid=4882 oauid=-1 ouid=0 oses=-1 ocomm="univention-mana"
type=PROCTITLE msg=audit(1610524812.243:7148742): proctitle=2F7573722F62696E2F707974686F6E322E37002F7573722F7362696E2F756E6976656E74696F6E2D6D616E6167656D656E742D636F6E736F6C652D736572766572007374617274
```

`ausearch` can be used to work with the log:

- `--interpret` converts the entries to a more human readable format.
- Use `--key Bug52518` to limit the output to those events tagged with the specified key.
- You can use `--start 12.01.2021 12:00:00` and `--end 13.01.2021 13:00:00` to limit the time frame.

## audit spam

In addition to our manually configured system call events many other services emit Audit events as well, which will clutter the log:

- `systemd` logs the start and stop of services.
- `PAM` logs session start and stop events.

They can be filtered out by using the `exclude` list in `/etc/audit/rules.d/bug52518.rules`:

```
-a always,exclude -F msgtype=PROCTITLE -F msgtype=SYSCALL -F msgtype=PATH -F msgtype=CWD -F msgtype=EXECVE -F msgtype=SERVICE_START -F msgtype=SERVICE_STOP -F msgtype=CONFIG_CHANGE
-a never,exclude
```

We add the named [message types](https://github.com/linux-audit/audit-documentation/blob/master/specs/messages/message-dictionary.csv) to **always** to white list them, all other types are black listed by catching them with **never**.

## Analysis

Use the above method we were able to capture (among others) the following event:

```console
# ausearch -i -a 7148742 -if audit.log.1
----
type=PROCTITLE msg=audit(13.01.2021 09:00:12.243:7148742) : proctitle=/usr/bin/python2.7 /usr/sbin/univention-management-console-server start 
type=OBJ_PID msg=audit(13.01.2021 09:00:12.243:7148742) : opid=4882 oauid=unset ouid=root oses=-1 ocomm=univention-mana 
type=SYSCALL msg=audit(13.01.2021 09:00:12.243:7148742) : arch=x86_64 syscall=kill success=yes exit=0 a0=0x1312 a1=SIGTERM a2=0x1 a3=0x20 items=0 ppid=1 pid=5965 auid=unset uid=root gid=root euid=root suid=root fsuid=root egid=root sgid=root fsgid=root tty=(none) ses=unset comm=univention-mana exe=/usr/bin/python2.7 key=Bug_52518
```

- `SYSCALL` tells us, that process `pid=5965` is dong a `syscall=kill` with `a1=SIGTERM` on process `0x1312`.
- From `PROCTITLE` we get the information, that the process doing this kill is `/usr/sbin/univention-management-console-server`.
- Form `OBJ_PID` we also get the information, that the process being killed is `pid=4882=0x1312`:
  It's a process running as `ouid=root` and it's abbreviated name is `ocomm=univention-mana`.

Stupidly the name is abbreviated, so we need to lookup the complete name.
As we also enabled tracing `sys_execve` we can use that to get the full command line.
This is easily done by running the following command filtering our log for the first event associated with that process ID `4882`:

```console
# ausearch -i -if audit.log.1  -p 4882 --just-one
----
type=PROCTITLE msg=audit(13.01.2021 09:00:12.243:7148745) : proctitle=/usr/bin/python2.7 /usr/sbin/univention-management-console-web-server start 
type=SYSCALL msg=audit(13.01.2021 09:00:12.243:7148745) : arch=x86_64 syscall=exit a0=EXIT_SUCCESS a1=0x7ff2669099c0 a2=0x3c a3=0x7c0 items=0 ppid=1 pid=4882 auid=unset uid=root gid=root euid=root suid=root fsuid=root egid=root sgid=root fsgid=root tty=(none) ses=unset comm=univention-mana exe=/usr/bin/python2.7 key=Bug_52518
```

Et voilà:
We only need `proctitle` to see, that `umc-server` is killing `umc-**web**-server`.
Those two processes are unrelated, as they have no immediate parent/child association.
Investigating other usages of `sys_kill` by using the same technique showed `umc-server` to also kill many other processes like `s4c`, `univention-portal` and all other services using `/usr/bin/python2.7`.

# gdb

Next we did a source code audit of `umc-server`, but found "nothing":
We only were able to construct an obscure case, were the new multi-process setup used `os.kill()`, but that mode was not enabled at both customers.
At least we know where to look and next used `gdb` to finally nail the pin:

Using `gdb -p $(pgrep -f univention-management-console-server)` we attached to the running process.
Next we told the debugger to break on the process using system call 62 (`sys_kill`) iself by doing a `catch syscall 62`.
We also want to cache the process being killed by `SIGTERM`, so we also trap on that using `catch signal 15`.
After resuming the program with `continue` we only had to wait some more to be interrupted again.

```
Thread 1 "univention-mana" hit Catchpoint 1 (signal SIGTERM), 0x00007fb21d8f5317 in kill () at ../sysdeps/unix/syscall-template.S:84
84	../sysdeps/unix/syscall-template.S: Datei oder Verzeichnis nicht gefunden.
```

Running `backtrace` immediately showed the C-level traceback, but you can even get more information when you install the Debian package `python-dbg`:
It does not only provide the "debugging symbols" for the Python interpreter, which map the machine code back to source code lines, but also extend `gdb` with some useful commands to debug Python programs from within `gdb` itself.
You can get a Python traceback by using `py-bt`:

```
(gdb) py-bt
Traceback (most recent call first):
  File "/usr/lib/python2.7/dist-packages/notifier/popen.py", line 270, in __killall
    os.kill(pid, signal)
  File "/usr/lib/python2.7/dist-packages/notifier/__init__.py", line 104, in __call__
    return self._function(*tmp, **self._kwargs)
  File "/usr/lib/python2.7/dist-packages/notifier/nf_generic.py", line 259, in step
    if not timer[CALLBACK]():
  File "/usr/lib/python2.7/dist-packages/notifier/nf_generic.py", line 304, in loop
    step()
  File "/usr/sbin/univention-management-console-server", line 232, in run
    notifier.loop()
  File "/usr/lib/python2.7/dist-packages/daemon/runner.py", line 186, in _start
    self.app.run()
  File "/usr/lib/python2.7/dist-packages/daemon/runner.py", line 267, in do_action
    func(self)
  File "/usr/sbin/univention-management-console-server", line 285, in 
    umc_daemon.do_action()
```

Nailed it:
`umc-server` is using [python-notifier](https://github.com/univention/python-notifier/commit/374993ed530d8b25f90baf8e708bc727cc2ef3a7), where its module for handling sub processes is killing the wrong "children".

And you also can have a look at Python local variables using `py-locals` (reformatted for improved readability):

```
self = <RunIt(
 _RunIt__stderr=None,
 stdout=None,
 _cmd=[
  '/usr/bin/python2.7',
  '/usr/sbin/univention-management-console-module',
  '-m', 'updater',
  '-s', '/var/run/univention-management-console/613-1610957913744.socket',
  '-d', '2',
  '-l', 'de_DE.UTF-8'],
 binary='/usr/bin/python2.7',
 _Process__dead=False,
 _Provider__signals={
  'finished': ,
  'killed': <Signal(_Signal__callbacks=[], name='killed') at remote 0x7fb1fc4e9890>},
 pid=23255,
 _name='python2.7',
 _Process__kill_timer=101,
 _RunIt__stdout=None,
 stderr=None,
 child=None,
 stopping=True,
 _shell=False) at remote 0x7fb1fc4ee990>
signal = 15
unify_name = 
appname = 'usrbinpython27'
cmdline_filenames = [
 '/proc/1-5,7-29,41-42,74-79,82-83,85,89,91,105,107,109-110,112,115,120-121,124-125/cmdline',
 '...(truncated)
cmdline_filename = '/proc/613/cmdline'
fd = 
cmdline = '/usr/bin/python2.7\x00/usr/sbin/univention-management-console-server\x00start\x00'
pid = 61
```

{% include abbreviations.md %}
