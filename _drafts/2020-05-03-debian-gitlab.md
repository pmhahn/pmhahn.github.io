---
layout: post
title: "Build Debian packages out-of-tree"
date: 2020-05-03 15:30:00  +0200
categories: debian
excerpt_separator: <!--more-->
---

For my employee [Univention](https://www.univention.de/) I want to setup a Continuous Integration system to build Debian packages.
We have been using our own build-system called *Repo-NG* based on [pbuilder](https://pbuilder-team.pages.debian.net/pbuilder/), which show their age.
Some years ago we switched from [Subversion](https://subversion.apache.org/) to [Git](https://git-scm.com/).
During that transition we already introduced [GitLab](https://gitlab.com/) internally, but used it merely for hosting our *git* repositories.
Recently I have investigated using the more advanced features like [Pipelines](https://docs.gitlab.com/ee/ci/pipelines/) to replace our aging [Jenkins](https://www.jenkins.io/) server.
Debian already has a [pipeline for building and testing Debian packages](https://salsa.debian.org/salsa-ci-team/pipeline/), which 

<!--more-->

