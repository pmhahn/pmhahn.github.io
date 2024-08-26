---
title: 'Python 105: wheels'
date: '2022-10-15T15:49:11+02:00'
author: pmhahn
layout: post
categories:
    - python
---

Python *wheels* are a distribution format for Python packages defined by [PEP-427](https://peps.python.org/pep-0427/). As long as your package is simple and just consists of a bunch of Python files, creating and shipping a *source only distribution* is probably fine. But as soon your Python package build process takes time or needs (many) other dependencies to build — or even worse — it needs some (C-)compiler and libraries plus their development headers, those packages become a pain.

```console
$ docker run --rm -ti python:3-slim pip install libvirt-python
Collecting libvirt-python
Downloading libvirt-python-8.8.0.tar.gz (236 kB)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ 236.5/236.5 KB 5.2 MB/s eta 0:00:00
Preparing metadata (setup.py) ... error
error: subprocess-exited-with-error

× python setup.py egg_info did not run successfully.
│ exit code: 1
╰─> [1 lines of output]
pkg-config binary is required to compile libvirt-python
[end of output]
```

Wheels are pre-compiled packages which can be uploaded to [PyPI](https://pypi.org/) in addition to the source files. As different operating systems (Windows, macOS, Linux) have different ABIs, Linux distributions use different C-libraries like [GLIBC](https://www.gnu.org/software/libc/) (RedHat, SUSE, Debian, Ubuntu, Gentoo) or [MUSL](https://musl.libc.org/) (Alpine), and different Python versions have different ABIs, wheels should be build for many to all combinations.

Luckily there is [cibuildwheel](https://gitlab.com/joerick/cibuildwheel), which does all that for you and can be integrated easily in a GitLab pipeline. For [libvirt-python](http://libvirt-python) I have created a [patch](https://git.knut.univention.de/phahn/libvirt-python/-/commit/4849dcb8157160948d8eef783359aa7d7610f05e), also for [python-ldap](https://git.knut.univention.de/phahn/python-ldap/-/commit/f7dca573f61f1e685f333d97d92d99a6087fde3a), which add *cibuildwheel*. The generated wheels are uploaded into a [GitLabs Package Registry](https://git.knut.univention.de/phahn/libvirt-python/-/packages/211), which must be explicitly added:

```console
$ docker run --rm -ti python:3-slim \
pip install libvirt-python==8.9.0 \
--extra-index-url https://git.knut.univention.de/api/v4/projects/686/packages/pypi/simple
$ docker run --rm -ti python:3-slim \
pip install python-ldap==3.4.3 \
--extra-index-url https://git.knut.univention.de/api/v4/projects/590/packages/pypi/simple
```

If you depend on the *latest* package version this is only a *temporary* solution as any new upstream version in PyPI will again lead to pip downloading the source package of the newer version instead of the older wheel. So on each new upstream version those repositories clones must be updated and a new build be triggered. Ask the upstream project if they can include [cibuildwheel into their pipeline](https://cibuildwheel.readthedocs.io/en/stable/setup/#configure-a-ci-service) and also upload the wheels.
As an alternative to using *latest* and to prevent your pipeline form failing when a new upstream version is released, consider pinning the Python package version as the example above already does and use a tool like [Dependabot on GitHub](https://github.com/dependabot/dependabot-core) or [GitLab Renovate](https://docs.renovatebot.com/modules/platform/gitlab/) to get notified of newer upstream versions and/or automatic merge requests to update your dependencies.

An alternative to install Python packages via *pip* is to use Debian (or Alpine) packages, which get be installed via

```bash
apt-get -qq update &&
apt-get -q --assume-yes python3-libvirt &&
apt-get clean
```

Those packages may be older than the latest version in PyPI, but at least they are consistent within the Debian distribution and are well tested.

PS: If there is demand for building more *wheels* it makes sense to create a separate group for that and move all projects there. That way a single group Package repository can be shared by all these projects, which simplifies and unifies the URL for this registry.
