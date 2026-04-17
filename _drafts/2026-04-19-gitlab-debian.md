---
title: 'Building Debian/Ubuntu package from GitLab pipeline'
date: '2026-04-17T14:28:00+02:00'
layout: post
categories: gitlab debian
excerpt_separator: <!--more-->
---

I'm using GitLab on a daily basis.
As a [Debian Developer](https://www.debian.org/) I like to automate building packages.
ebian itself runs its own instance called [Salsa](https://salsa.debian.org/).
The maintain their own excellent [CI pipeline](https://salsa.debian.org/salsa-ci-team/pipeline), which can do and does many things:
- (cross-)build the Debian package from the git source for multiple architectures
- run Debian's [lintian](https://wiki.debian.org/Lintian) to find packaging issues
- upload the package files into a (temporary) package repository (using [aptly](https://www.aptly.info/)), so other pipelines can use them as build dependencies
- run [diffoscope](https://diffoscope.org/) between package version to make differences visible
- run [piuparts](https://piuparts.debian.org/) to test package installation, upgrade and removal
- run [autopkgtest](https://wiki.debian.org/autopkgtest) to run test suites
- build the package twice to find incomplete rules to clean up the source package after a build
- build downstream packages to find API/ABI breakages
- …

While you can use the pipeline on your own infrastructure or even on [gitlab.com](https://gitlab.com/), it requires some setup to get everything to work.
Most often I don't need every feature of the Debian CI pipeline.
As such I'd like to go with a simpler version.

<!--more-->

# A very simple Debian package

-   `hello`: An example script to package

    ```sh
    #!/bin/sh
    echo 'Hello World!'
    ```

-   [`debian/control`](https://www.debian.org/doc/debian-policy/ch-controlfields.html): 

    ```
    Source: hello
    Section: misc
    Priority: optional
    Maintainer: Philipp Matthias Hahn <pmhahn@debian.org>
    Build-Depends: debhelper-compat (>= 13),
    Standards-Version: 4.0.0

    Package: hello
    Architecture: all
    Depends: ${misc:Depends}
    Description: The famous hello-work program
    ```

-   [`debian/copyright`](https://www.debian.org/doc/manuals/maint-guide/dreq.en.html#copyright)

    ```
    Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
    Upstream-Name: hello
    Upstream-Contact: Philipp Matthias Hahn <pmhahn@debian.org>
    Source: https://pmhahn.de/
    
    Files: *
    Copyright: 2026 Philipp Matthias Hahn <pmhahn@debian.org>
    License: GPL-2+
    ```

-   [`debian/changelog`](https://www.debian.org/doc/manuals/maint-guide/dreq.en.html#changelog)

    ```
    hello (1.0.0) unstable; urgency=medium

      * Initial release.

    -- Philipp Matthias Hahn <pmhahn@debian.org>  Sat, 18 Apr 2026 07:49:11 +0200
    ```

-   [`debian/rules`](https://www.debian.org/doc/manuals/maint-guide/dreq.en.html#rules)

    ```
    #!/usr/bin/make -f
    @:
    	dh $@
    ```

    It will ask me for the next version number and will then create that release.

-   [`debian/source/format`](https://www.debian.org/doc/manuals/maint-guide/dother.en.html#sourcef)

    ```
    3.0 (native)
    ```

-   [`debian/.gitignore`](https://git-scm.com/docs/gitignore)

    ```
    /files
    /*.debhelper
    /*.log
    /*.log.debhelper
    /*.substvars
    /*/
    !/patches/
    !/source/
    ```

# The GitLab pipeline

```yaml
stages:
  - prepare
  - build
  - publish

build docker container:
  stage: prepare
  script:
    - apt-get -qq update
    - apt-get -qq -y build-depends .

build debian package:
  stage: build
  variables:
    DEBIAN_FRONTEND: noninteractive
  script:
    - >
      printf "%s=%s\n" >env
      src "$(dpkg-parsechangelog -SSource)"
      ver "$(dpkg-parsechangelog -SVersion)"
      dist "$(dpkg-parsechangelog -SDistribution)"
      time "$(dpkg-parsechangelog -STimestamp)"
    . .env
    - find -exec touch -m -h -c -d "@${SOURCE_DATE_EPOCH:-$time}" {} +
    - dpkg-buildpackage --no-sign
    - mkdir dist
    - dcmd mv -t dist/ ../*.changes
  artifacts:
    paths:
      - dist/*.deb
```
1. This saves some variables into a `dotenv` file, so later jobs may access that data.
2. It updates all files to have a fixed time-stamp: The `SOURCE_DATE_EPOCH` defaults to the time stamp of the latest entry of `debain/changelog`. Without this the package cannot be build reproducible.
3. `dpkg-buildpackage` puts the generated files into the parent directory. To be saved as an archive they must be moved into a sub-directory of `CI_PROJECT_DIR`. This is done by using <man:dcmd(1)> from [devscripts](https://tracker.debian.org/pkg/devscripts).

```yaml
publish debian package:
  stage: publish
  needs:
    - job: build debian package
  script:
    - |
      URL="${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/debian_distributions?codename=${dist}"
      curl --fail --head "$URL" ||
        curl --fail --request POST --user "PRIVATE-TOKEN:${CI_JOB_TOKEN}" "$URL"
    - >
      printf >dput.cf '%s\n'
      '[gitlab]'
      'method = https'
      "fqdn = Job-Token:${CI_JOB_TOKEN}@${CI_SERVER_FQDN}"
      "incoming = /api/v4/projects/${CI_PROJECT_ID}/packages/debian"
    - >
      dput
      --config=dput.cf
      --force
      --unchecked 
      --no-upload-log
      gitlab
      dist/*.changes
```
This publishes the package to [GitLab's Debian package registry](https://docs.gitlab.com/user/packages/debian_repository/).
As of GitLab 18.10 this is still under development and must be enabled via the feature flag.

{% include abbreviations.md %}
