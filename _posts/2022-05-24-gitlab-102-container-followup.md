---
title: 'Gitlab 102: container followup'
date: '2022-05-24T08:41:47+02:00'
layout: post
categories: gitlab container
---

Continuing [Gitlab 101: Container usage]({% post_url 2022-03-10-gitlab-101-container-usage %}) there are some news:

## New certificates

Helpdesk has replaces the SSL/TLS certificates of several hosts with a wildcard certificate from [Certum](https://www.certum.eu/):

- git.knut.univention.de
- \*.gitpages.knut.univention.de
- gitregistry.knut.univention.de
- docker-registry.knut.univention.de

This simplifies most pipelines:
you no longer have to insert the `ucs-root-ca.crt` and call [update-ca-certificates](man:update-ca-certificates(8)).
Mind that you still must have `ca-certificates` installed as it contains the root CAs.
Most docker images already contain it, but some like [curl](https://hub.docker.com/r/curlimages/curl) are too minimalistic and you have to add it on-top yourself.

PS: The certificate for `*.gitpages.knut.univention.de` has also been updated by me and should work again.
It still uses a certificate from our internal KNUT CA, but the [Subject Alternative Names](https://en.wikipedia.org/wiki/Subject_Alternative_Name) (SAN) haven been re-ordered:
After much searching I found out, that Mozilla Firefox **aborts** checking following SANs after encountering an *invalid* one:
In our case it considered `*.gitpages` a TLD and never reached `*.gitpages.knut.univention.de`.
More details at  [Bug #54698](https://forge.univention.org/bugzilla/show_bug.cgi?id=54697#c3).

## Pipeline fragments

Instead of copy-pasting the **current** best practice into each pipeline Gitlab offers an [include mechanism](https://docs.gitlab.com/ee/ci/yaml/#include) to include other pipeline YAML fragments:
They can be [local in your current project](https://docs.gitlab.com/ee/ci/yaml/#includelocal), from a [different project](https://docs.gitlab.com/ee/ci/yaml/#includefile), an [arbitrary URL](https://docs.gitlab.com/ee/ci/yaml/#includeremote), or a [Gitlab shipped fragment](https://docs.gitlab.com/ee/ci/yaml/#includetemplate) supporting [many use-cases](https://gitlab.com/gitlab-org/gitlab/-/tree/master/lib/gitlab/ci/templates) out-of-the-box and which are used by [Gitlab Auto DevOps](https://docs.gitlab.com/ee/topics/autodevops/).

The *Development Infrastructure Service Team* provides some pipeline fragments as part of our Docker services project itself for anyone to use:

- `kaniko.yml`
  [Kaniko](https://github.com/GoogleContainerTools/kaniko) is an alternative for building Docker images and does not require a privileged DinD daemon.
  In most cases it is noticeably faster as Kaniko can be used on the newer Gitlab runners hosted by PlusServer, which are much closer to our main Gitlab instance also running in the same network.
- `dind.yml`
  This prepares the pipeline to use *Docker in docker*, if you still have to use it.
  Please consider using `kaniko.yml` instead as DinD required privileged Gitlab runners, which are phased out because the existing ones are currently running on servers hostes at Briteline, which require each network package to go over the internet through our OpenVPN tunnel, which still has performance issues:
  We seldom observe pipeline failures because pushing an image from Bremen to `docker-registry.knut` is aborted because pushing 200 MiB takes longer than one hour!
- `skopeo.yml`
  [Skopeo](https://github.com/containers/skopeo) is a tool to work with docker images and can be used to copy images between registry.
  As such it can also be used to tag docker images without pulling, tagging, pushing them to/on/from the local host but directly remotely.

Depending on which Docker container registry you're going to use most job templates defined in those pipeline fragment files exist in two variants:

- One using our legacy `docker-registry.knut.univention.de` not requiring any authentication.
  They carry the suffix `_knut`.
- One using `gitregistry.knut.univention.de` from Gitlab.

Using them simplifies a pipeline to this:
```yaml
include:
 – project: univention/dist/docker-services
   file: kaniko.yml

job to build my docker image:
 extends: .kaniko
```

This will build and tag a docker image depending on the git branch:

1. The variable `IMAGE_TAG` always wins and can be overwritten in the pipeline itself via a [pipeline variable](https://docs.gitlab.com/ee/ci/yaml/#variables)
2. For a commit on the *default branch* (`$CI_DEFAULT_BRANCH`) `latest` is used
3. For a pipeline triggered by [git tag](https://git-scm.com/book/en/v2/Git-Basics-Tagging) that git-tag is also re-used as the docker-tag
4. Otherwise the git-branch-name is used as the docker-tag prefixed with `branch-`

Unless `IMAGE_TAG` is used `CI_REGISTRY_IMAGE` is used as the image name, which derives from the Gitlab project path;
the `.kaniko_knut` defaults to `docker-registry.knut.univention.de/knut/$CI_PROJECT_NAME` instead.

## Why use that fragment instead of copy/paste

The DevOps philosophy is to *do it right once*, so that everyone can participate in improving that one implementation instead of having many fragmented and divering ones.
Changing the certificate is a prime example for that:
With the copy/paste approach we now have to fix many pipelines, where with the `kaniko.yml` fragment we had to change it only there.
Similar to that we discovered a problem with the [caching approach used by kaniko](https://cloud.google.com/build/docs/kaniko-cache), which should not be used with `docker-registry.knut` because it does not support automatic image expiry and lead to the file system getting filled very fast.
Fixing that in `kaniko.yml` once was easy, but all other pipelines have not been reviewed yet and might still result in extra work to be done.
So please contribute to improving these fragments and use them instead of implementing your own solution.

{% include abbreviations.md %}
