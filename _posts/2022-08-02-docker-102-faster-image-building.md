---
title: 'Docker 102: Faster image building'
date: '2022-08-02T08:09:16+02:00'
layout: post
categories: container
---

[Gitlab 103: Kaniko image building]({% post_url 2022-06-04-gitlab-103-kaniko-image-building %}) described, how to build Docker respective OCI images using [Kaniko](https://github.com/GoogleContainerTools/kaniko).
Compared to _Docker-in-Docker_ it has one drawback:
speed — at least when your `Dockerfile` contains `RUN` commands, which take a long time like doing an `apt-get install` with many or large packages.

This was a problem for the UCS pipeline for building `ucslint`:
downloading and installing the Debian package dependencies took most of the 3:24 minutes, while adding the latest version from git was done under a second.

Here are some tricks on how to improve your image and how to speed up the build process for them.

## Previous layer caching

If you carefully read [Docker 101: Container basics]({% post_url 2022-03-10-gitlab-101-container-usage %}) you will notice the following behavior, when an image is built:
for each layer `docker` tracks, which underlying layer gets used and what command is used to build the next layer.
    This can be seen by running the `docker image history` command:

```console
# cat temp/Dockerfile
FROM debian:bullseye-slim
RUN touch /stamp
ADD Dockerfile /
# docker build -t empty temp/
Sending build context to Docker daemon  2.048kB
Step 1/3 : FROM debian:bullseye-slim
 ---> bfbec70f8488
Step 2/3 : RUN touch /stamp
 ---> Running in ce41dd79a586
Removing intermediate container ce41dd79a586
 ---> d3831cac03c4
Step 3/3 : ADD Dockerfile /
 ---> e1eddc7fc1d8
Successfully built e1eddc7fc1d8
Successfully tagged empty:latest
# docker image history empty
IMAGE          CREATED          CREATED BY                                      SIZE      COMMENT
e1eddc7fc1d8   29 seconds ago   /bin/sh -c #(nop) ADD file:02a0d6b33cf854ad6…   60B
d3831cac03c4   29 seconds ago   |0 /bin/sh -c touch /stamp                      0B
bfbec70f8488   3 months ago     /bin/sh -c #(nop)  CMD ["bash"]                 0B
               3 months ago     /bin/sh -c #(nop) ADD file:8b1e79f91081eb527…   80.4MB
```

If an image is rebuild `docker` first checks, if the previous layer can be reused:
It is when the both the base layer and the `Dockerfile` command are unchanged.

- For `ADD` and `COPY` the content of the copied files is also checked for changes, so that changes in the context will end up in your rebuilt.
- On the other hand commands like `RUN` may not get re-executed:
The `touch`-command above defaults to the current time.
The file `/stamp` above will carry the time-stamp, when the image was first created.
On subsequent builds `docker` detects, that the `RUN` command is unchanged and will reuse the original layer with the original time-stamp.

You can see this, when you re-built the image from above:

```console
# echo >> temp/Dockerfile
# docker build -t empty temp/
Sending build context to Docker daemon  2.048kB
Step 1/3 : FROM debian:bullseye-slim
 ---> bfbec70f8488
Step 2/3 : RUN touch /stamp
 ---> Using cache
 ---> d3831cac03c4
Step 3/3 : ADD Dockerfile /
 ---> 75816d4c2d28
Successfully built 75816d4c2d28
Successfully tagged empty:latest
# docker image history empty
IMAGE          CREATED          CREATED BY                                      SIZE      COMMENT
75816d4c2d28   6 seconds ago    /bin/sh -c #(nop) ADD file:ad97b7631b5bf96d6…   61B
d3831cac03c4   13 minutes ago   |0 /bin/sh -c touch /stamp                      0B
bfbec70f8488   3 months ago     /bin/sh -c #(nop)  CMD ["bash"]                 0B
               3 months ago     /bin/sh -c #(nop) ADD file:8b1e79f91081eb527…   80.4MB
```

`docker` reuses the original layer `d3831cac03c4` and only the top-most layer gets replaced from `e1eddc7fc1d8` to `75816d4c2d28`.

Therefore it is important to write efficient and correct `Dockefiles`:

1. Put steps which change infrequently first like installing base packages from Debian or PyPI:
   If you dependency do not change often these layers stay unchanged for a long time and can be re-used many time speeding up many builds.
2. Add your ever changing code as late as possible:
   That way only the last few layers need to be re-build each time.
3. Be careful with volatile data and when the current time is important:
   Your `apt-get update && apt-get install` command may not pick up the latest package versions as `docker` decides to re-use the layer from a previous build.
4. Minimize the number of `RUN` commands:
   each command adds an additional layer to your image, which must be downloaded.
   It also may lead to strange caching issues and may increase the net image size unnecessarily due to temporary data:
   For example `apt-get update` will downloaded the Debian package index files and store them below `/var/lib/apt/lists/`.
   Your final image probably will not need them as you don't expect your users to do a `apt-get install` within your image.
   On the other hand you will get into trouble yourself, when you re-build the image and do that `apt-get install` yourself in a later step as your index files might be out-of-date by than and do no longer match what is on the ever changing Debian repository servers.
5. If you ever get into caching issues (on your own host) delete any previous image from your `dockerd` by using `docker image rm …`:
   That way the old layers will be deleted too and `docker build` will no longer find the layers cached locally.

## Docker-in-Docker

With *Docker-in-docker* you get a pristine `dockerd` for each build:
It is empty and thus does not have any previous images and thus layers cached.
Thus a build will always starts from scratch, which might be slow.
Therefore you can use `docker build --cache-from=…` to load a previous version of your image, which will pull in all the old layers for caching.

## Kaniko

The architecture of Kaniko is vastly different as there is no central `dockerd` for caching any layers.
But [2 caching modes](https://github.com/GoogleContainerTools/kaniko#caching) are supported:

- `--cache=true` enabled *Caching Layers*, where `kaniko` builds an artificial image, which can be pushed to a remote image registry.
  These are no regular OCI images:
  they re-use the data structure and infrastructure for images, but you cannot execute them with `docker run` or similar.
  You **must not** push these images to `docker-registry.knut.univention.de` as that registry has no automatic cache cleanup mechanism to remove old and unused layers/images.
  Use `gitregistry.knut.univention.de` instead but make sure to setup [Cleanup policy](https://docs.gitlab.com/ee/user/packages/container_registry/reduce_container_registry_storage.html) below `Setting → Packages and Registries → Clean up image tag` with a sensible retention period for images named `cache`.
  A separate registry can be specified via the option `--cache-repo`.
  By default Kaniko does not cache *COPY* layers, which must be enabled explicitly via the option `--cache-copy-layers` if desired.
- With *Caching Base Images* layers are cached locally in a directory, but this is a pain to setup:
  it requires an extra step where a separate command has to be used to *warm the cache*.
  This is not integrated into Kaniko itself and extra care has to be taken to not create any concurrency issues.

Currently caching is not enabled by default in our `kaniko.yml` as setting up the cache retention policy is not automated.
You can enable it yourself after reading the warning above by passing the extra argument via the pipeline variable `KANIKO_ARGS: --cache=true`.
It is already documented in `kaniko.md`.

See [univention/ucs>ucslint](https://github.com/univention/univention-corporate-server/blob/release-5.0-4/.gitlab-ci.yml#L87) for a real-world example.

{% include abbreviations.md %}
