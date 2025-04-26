---
title: 'docker run user'
date: '2020-09-12T14:57:28+02:00'
layout: post
categories: container
---

`docker` is nice to run application commands as you can put them into a container, which also includes the required dependencies.
This saves you from cluttering your notebook with a multitude of strange packages from Debian, PyPI, golang, … We're already using this for running our DocBook toolchain or `ucslint` and are in the process to convert `ucs-ec2-tools` to this.

Most of these images require access to your files:
`ucslint` for checking your code, `ucs-ec2-tools` for your `.cfg` and associated files, `DocBook` to your documentation.
In most cases read-only access is okay for example when used by GitLab, as there the code is checked out from `git` by the `gitlab` user and the Docker images runs as user `root`, so inside the Docker container everything is allowed (but still constrained by the container).
Files created there are usually uploaded as artifacts from within the container by the same user, so the complete working environment can simply be purged after everything is done.
(Actually the working space may get re-used by later builds).

If on the other hand you're using those same images on your notebook, you have a problem:

For the container to have access to your local files you usually pass in a potion of your local file system as a **volume**:

```bash
docker run –rm -ti -v "$PWD":/work -w /work debian touch file
```

The command inside the container will run as user `root` and thus the newly created file inside your current working directory will be owned by user `root`.
Deleting that file is normally not a problem as you're hopefully the owner of your current working directory, but this becomes a nightmare easily when you create a deeply nested directory tree from within the container:
Then the directories are owned by `root` and if they are not empty, you will not be allowed to delete them as your regular user.

## USER

The user `root` is just the default:
Using the option [USER](https://docs.docker.com/engine/reference/builder/#user) statement inside a `Dockerfile` allows you to change the default.
You're allowed to use any numeric ID or any symbolic ID known inside the image in `/etc/passwd`.
But you already have to know the ID when you **build** the image, so you can no longer share the image between users needing different UIDs (if they want to write files).

## –user

Actually this is not 100% right as you can use the option `--user` to still run the container with a different user ID, namely your (numeric) user and group ID:

```bash
docker run –rm -ti -v "$PWD":/work -w /work -u "$(id -u):$(id -g)" debian touch file
```

This **overwrites** any usage of `USER` from `Dockerfile`.
The newly created file is now owned by your user.

But you probably will notice one annoyance:

```console
# docker run –rm -ti -v "$PWD":/work -w /work -u "$(id -u):$(id -g)" debian id
uid=2260 gid=2260 groups=2260
```

You will notice that the **numeric** ID is not associated with a **symbolic** user our group entry within the container because there are no entries in `/etc/passwd` or `/etc/group`.
For many programs that is not a problem because the numeric IDs are sufficient for the programs to function.

But any program doing a `getent passwd` or `getent group` will get no value and might do strange things:
`ssh` will not start, `fop` creates directories named `?`, `sudo` refuses to work at all, ….
Fixing this is not easy as you're already a *normal* user within the container, so you can no longer call `adduser` to create that user inside the container.
As `sudo` refuses to work, you're stuck.

## `nss_wap`

While searching for a solution to this problem I stumbled over [`nss_wrapper`](https://cwrap.org/nss_wrapper.html), which is an `LD_PRELOAD` library.
By using it you can intercept the `getent` calls to `glibc` and resolve them using alternative files `passwd` and `group` under your control.
The _DokBook Docker image_ for example uses this and creates a copy of those two files on the fly by appending two dummy mappings if the user or group are missing.
This script is then used as the [ENTRYPOINT](https://docs.docker.com/engine/reference/builder/#entrypoint) in the `Dockerfile`, which makes those Name Service Switch (`NSS`) using programs happy.

## User Name Space

For completeness you can also re-configure your `dockerd` to allow the usage of [userns-remap](https://docs.docker.com/engine/security/userns-remap/#enable-userns-remap-on-the-daemon).
This allows you to map users within the container to user IDs outside the container on your host.
As this requires changing the daemon configuration on probably many hosts, I did not pursue this path.

{% include abbreviations.md %}
