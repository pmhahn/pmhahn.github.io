---
title: 'GitLab 105: Beware global variables'
date: '2022-10-13T12:59:59+02:00'
layout: post
categories: virt git
---

Today UCS@school had an interesting issue, where the Kelvin REST API pipeline failed, specifically the build job building a Docker image using [Kaniko]({% post_url 2022-06-04-gitlab-103-kaniko-image-building %}).
This gist of the error was a permission error:

> error checking push permissions …
> checking push permission for "gitregistry.knut.univention.de/univention/components/ucsschool-kelvin-rest-api:branch-dtroeder-21-kelvin-add-support-for-head-schools-school-name"
> UNAUTHORIZED: authentication required

Pushing to a Docker registry hopefully always requires authentication:
gitregistry.knut.univention.de does, but actually docker-registry.knut.univention.de does not.
Because of this the pipeline contains a line of code to setup credentials in `~/.docker/config.json`:

```bash
echo "{\"auths\":{\"${CI_REGISTRY}\":{\"auth\":\"$(printf "%s:%s" "${CI_REGISTRY_USER}" "${CI_REGISTRY_PASSWORD}" | base64 | tr -d '\n')\"}}}" >/kaniko/.docker/config.json
```

But this did no longer worked since the start if the week.

What happened here is that the [.gitlab-ci.yml](https://github.com/univention/ucsschool-kelvin-rest-api/blob/main/.gitlab-ci.yml#L2) includes the Sphinx _pipeline fragment_ which overwrite `CI_REGISTRY` with `docker-registry.knut.univention.de`.
So the code above was setting up authentication for that other registry, but actually was pushing to `CI_REGISTRY_IMAGE` which was already calculated before `CI_REGISTRY` was changed.
So both variables no longer matched which lead to that strange error.

The fix was to not overwrite `CI_REGISTRY` but use some other variable unique to that fragment.

There are many [CI variables defined by GitLab](https://docs.gitlab.com/ee/ci/variables/predefined_variables.html), most of wich start with `CI_…`:
they are provided by GitLab and contain information which might be useful to your pipelines.
You can use them for reading but should not overwrite them as we just learned.

If you write [pipeline fragments]({% post_url 2022-05-24-gitlab-102-container-followup %}) be very careful if you define global variables:
prefix them with some prefix unique to you fragment to prevent spilling into the global namespace and overwriting things there.

Someone™ should collect these best practices for writing GitLab pipelines and put them somewhere where everyone can look them up and follow them …

PS: [Ceterum censeo](https://en.wikipedia.org/wiki/Carthago_delenda_est) stop using [docker-registry]({% post_url 2022-03-10-gitlab-101-container-usage %}) but switch to [GitLab container gitregistry](https://docs.gitlab.com/ee/user/packages/container_registry/).

PPS: If you have to debug tricky GitLab pipeline issued gibt [`CI_DEBUG_TRAGE: "true"`](https://docs.gitlab.com/ee/ci/variables/#debug-logging) a try.
This is similar to *bash -x* and might reveal credentials, so be careful.

{% include abbreviations.md %}
