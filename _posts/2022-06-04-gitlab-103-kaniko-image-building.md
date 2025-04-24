---
title: 'Gitlab 103: Kaniko image building'
date: '2022-06-04T11:37:35+02:00'
layout: post
categories: virt git
---

[Gitlab 102: container followup]({% post_url 2022-05-24-gitlab-102-container-followup %}) briefly describes building Docker images with [Kaniko](https://github.com/GoogleContainerTools/kaniko) instead of _Docker in docker_. Let's take a look at some common practices to build images.

1. Always include the _Kaniko_ fragment:
    ```yaml
    include:
     – project: univention/dist/docker-services
       file: kaniko.yml
    ```
2. This fragments defines multiple base jobs, that you can use with the `extends` keyword in your pipeline definition in your `.gitlab-ci.yml` file. They use different Docker registries `$CI_REGISTRY` and image names `$CI_REGISTRY_IMAGE` by default. GitLab already defines these [predefined variables](https://docs.gitlab.com/ee/ci/variables/predefined_variables.html). Some jobs re-define them and you can them in the `variables` section of each job in your pipeline definition.
3. The Kaniko fragment above defines `latest` for the *docker tag*, when the build runs in the `$CI_DEFAULT_BRANCH`. Otherwise the Kaniko fragments assigns the `$CI_COMMIT_TAG` for tags and `$CI_COMMIT_REF_NAME` for feature branches.

## Default build with Gitlab docker registry

```yaml
build my docker image:
 extends: .kaniko
```

Registry `CI_REGISTRY`
: gitregistry.knut.univention.de
Image `CI_REGISTRY_IMAGE`
: `$CI_REGISTRY/$CI_PROJECT_PATH_SLUG`

## Default build with docker-registry

```yaml
build my docker image:
 extends: .kaniko_knut
```

Registry `CI_REGISTRY`
: docker-registry.knut.univention.de
Image `CI_REGISTRY_IMAGE`
: `$CI_REGISTRY/knut/$CI_PROJECT_NAME`

## Customized base image name

You can overwrite the `$CI_REGISTRY_IMAGE` variable for the job

```yaml
build my docker image:
 extends: .kaniko_knut
 variables:
  CI_REGISTRY_IMAGE: "$CI_REGISTRY_IMAGE/my-image"
```

## Customized complete image name and tag

Overwrite `$IMAGE_TAG` for the job, for example to re-use the git hash as the docker tag:

```yaml
build my docker image:
 extends: .kaniko_knut
 variables:
  IMAGE_TAG: "$CI_REGISTRY_IMAGE/my-image:$CI_COMMIT_SHA"
```

### Explicit tagging

if you also want to explicitly re-tag the image with some extra tag, e.g. latest, use [Skopeo](https://github.com/containers/skopeo) and include `skopeo.yml` as fragment and add a second job for tagging:

```yaml
include:
 – project: univention/dist/docker-services
   file: kaniko.yml

tag my docker image:
 extends: .skopeo_knut
 needs:
  – job: build my docker image
 variables:
  GIT_STRATEGY: none
 rules:
  – if: "$CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH"
 script:
  – /bin/skopeo copy docker://$IMAGE_TAG docker://$CI_REGISTRY/my-image:latest
```

What happens here is that the first job writes a [dotenv artifact](https://docs.gitlab.com/ee/ci/yaml/artifacts_reports.html#artifactsreportsdotenv) defining the environment variable `$IMAGE_TAG`, which the second jobs depends on and includes.
This is then used to specify the source image.

## Building an image and use it

You can use the same technique of passing the `$IMAGE_TAG` to build an image in one job and then use that image for a second job:

```yaml
build my docker image:
 extends: .kaniko_knut

job using the docker image:
 needs:
  – job: build my docker image
 image: "$IMAGE_TAG"
```

### Conditional build

If building the Docker image is gated behind some `rules:`, the environment variable `$IMAGE_TAG` remains unset.
For that case create an additional job, which is always executed and which calculates the image name used by all following jobs:

```yaml
stages:
 – Prepare
 – Build
 – Use

prepare my docker image:
 stage: Prepare
 extends: .kaniko_pre
 variables:
  CI_REGISTRY_IMAGE: "$CI_REGISTRY/my-image"

build my docker image:
 stage: Prepare
 extends: .kaniko_knut
 variables:
  CI_REGISTRY_IMAGE: "$CI_REGISTRY/my-image"
 rules:
  – changes:
     – Dockerfile

use my docker image:
 stage: Use
 needs:
  – job: prepare my docker image
 image: "$IMAGE_TAG"
```

## Variable overrides

`KANIKO_ARGS`
: can be used to pass arbitrary extra arguments to kaniko, e.g. `--cache=true --cache-repo=$CI_REGISTRY_IMAGE/cache"` for enabling [layer caching](https://cloud.google.com/build/docs/kaniko-cache).
`KANIKO_BUILD_CONTEXT`
: defaults to `$CI_PROJECT_DIR` but can be used to specify a different build context directory.
`DOCKERFILE_PATH`
: can be used to specify an alternative path for the `Dockerfile`. For kaniko that file must be inside the directory `$KANIKO_BUILD_CONTEXT`.
`IMAGE_TAG`
: can be used to specify the full image name including registry and tag.

## Known issues

- Dockerfile must be inside context directory.
- Handling of special files is broken [Kaniko#960](https://github.com/GoogleContainerTools/kaniko/issues/960)

{% include abbreviations.md %}
