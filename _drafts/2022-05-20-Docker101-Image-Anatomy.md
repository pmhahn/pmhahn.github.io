---
layout: post
title: "Docker 101: Anatomy of a Docker image"
date: 2022-05-20 12:00:00  +0100
categories: linux virt
excerpt_separator: <!--more-->
---

Following <a href="https://hutten.knut.univention.de/blog/gitlab-101-container-usage/">Gitlab 101: Container usage</a> let's have a look at *Docker images*, also known as [OCI](https://www.opencontainers.org/) (Open Container Initative) images. If you want to know the gory details have a look at the [image specification](https://github.com/opencontainers/image-spec/blob/main/spec.md) yourself.

When working with images multiple layers are involved:

1. A **directory** to map human usable names to a sha256s
2. A **manifest** to describe an image
3. A **configuration** containing instructions to setup the environment for an image
4. A number of **layers** containing the files of the image

<!--more-->

# Docker registry layout
## Directory
At the top you have a **directory** like [Docker Hub](https://hub.docker.com/) or [Quay.io](https://quay.io/) or a [Gitlab container registry](https://docs.gitlab.com/ee/user/packages/container_registry/) or a [private docker registry](https://docs.docker.com/registry/) like `docker-registry.knut` or [docker.software-univention.de](https://docker.software-univention.de/v2/_catalog). They map **image names and tags** to an image, which is known by its sha256 hash. This mapping is **volatile** and can be updated any time. If you want [reproducible build](https://reproducible-builds.org/) you should do this lookup once manually and then specify the sha256 instead of the name.

## Manifests
An image is described by a **manifest**, which names the *architecture* the image is for, references a *configuration* document by sha256 and all the stacked *layers* by there hash required to assemble the complete image.
Multiple architectures can be supported by building the image for each architecture and pushing them to the same name. This creates an additional manifest for each architecture and also a **manifest-list**, which references all of them and itself is pointed to in the directory.

## Layers
Each **layer** consists of a set of files, which are additive by nature. They have to be stacked over each other in the right order to assemble the complete file system. Each layer is read-only to prevent any kind of modification. A writable layer is put on top last to allow multiple containers to use the same image without interfering with each other. Changing an existing file will trigger a *pull-up*, where the original file is copied from its read-only-layer to the writable top-layer first before the write occurs. Deleting a file will simply create a *white-out* entry in the top-layer, which will hide the underlying file, so this actually will not free any space.

# Examples
You need some tools like `curl`, `jq` and most importantly `docker`.

## Pulling an image
Here is what happens when you do a `docker pull debian` on the command line.
First the *repository tag* is normalized:

1. As no repository URL is included (everything before the first slash including at least one dot), `docker` defaults to `docker.io`.
2. The tag does not include any slash, `docker` prefixes it automatically with `library/`. This is only done for *Docker Hub*, but not for `registry.example.com/name`.
3. As no *tag* is specified `:latest` is used by default.

The canonical image name is thus `docker.io/library/debian:latest`, which results into a request to `https://registry-1.docker.io/v2/**library/debian**/manifests/**latest**`.
Navigating there with a web browser will just redirect you to Docker Hub, as you must request the right *content-type* `application/vnd.docker.distribution.manifest.v2+json`. Docker.hub also requires *Bearer authentication*:

```bash
AUTH_SERVICE='registry.docker.io'
AUTH_SCOPE='repository:library/debian:pull'
TOKEN=$(curl --fail --silent --show-error --location \
  "https://auth.docker.io/token?service=$AUTH_SERVICE&amp;scope=$AUTH_SCOPE" |
  jq --raw-output '.token')
curl -fsSL -D ./manifest.head -o ./manifest.body \
  --header "Authorization: Bearer $TOKEN" \
  -H 'Accept: application/vnd.docker.distribution.manifest.v2+json' \
  'https://registry-1.docker.io/v2/library/debian/manifests/latest'
```

This returns the following headers, which are saved to the local file `./manifest.head`:

```
HTTP/1.1 200 OK
content-length: 529
content-type: application/vnd.docker.distribution.manifest.v2+json
docker-content-digest: sha256:da1a55850480753941bb8aff55935742562aca344adac8544799e6551b4fe802
docker-distribution-api-version: registry/2.0
etag: "sha256:da1a55850480753941bb8aff55935742562aca344adac8544799e6551b4fe802"
date: Fri, 20 May 2022 16:35:30 GMT
strict-transport-security: max-age=31536000
ratelimit-limit: 100;w=21600
ratelimit-remaining: 100;w=21600
docker-ratelimit-source: 84.144.152.133
```

Let's look at some of those lines in more detail:


1. ```
docker-content-digest: sha256:da1a55850480753941bb8aff55935742562aca344adac8544799e6551b4fe802
```

   This is the stable hash of the manifest; it matches the sha256 checksum of the returned content and can be verified using `sha256sum ./manifest.body`, which will calculate the same checksum from the content.

2. ```
ratelimit-limit: 100;w=21600
ratelimit-remaining: 100;w=21600
docker-ratelimit-source: 84.144.152.133
```

   Since [November 2020](https://docs.docker.com/docker-hub/download-rate-limit/) Docker Hub only allows 100 directory lookup per 6 hours for anonymous users. These headers can be used to get the maximum (100) and remaining (100) number of requests.

The content is the manifest for that image, which is saved in the local file `./manifest.body`:

```javascript
{
   "schemaVersion": 2,
   "mediaType": "application/vnd.docker.distribution.manifest.v2+json",
   "config": {
      "mediaType": "application/vnd.docker.container.image.v1+json",
      "size": 1463,
      "digest": "sha256:c4905f2a4f97c2c59ee2b37ed16b02a184e0f1f3a378072b6ffa9e94bcb9e431"
   },
   "layers": [
      {
         "mediaType": "application/vnd.docker.image.rootfs.diff.tar.gzip",
         "size": 54945622,
         "digest": "sha256:67e8aa6c8bbc76b1f2bccb3864b0887671833b8667dc1f6c965fcb0eac7e6402"
      }
   ]
}
```

Before the image can be used, two more requests are required:

1. First the *configuration* must be fetched:
   ```bash
curl -fsSL -D ./config.head -o ./config.body \
  -H 'Accept: application/vnd.docker.container.image.v1+json' \
  'https://registry-1.docker.io/v2/library/debian/blobs/c4905f2a4f97c2c59ee2b37ed16b02a184e0f1f3a378072b6ffa9e94bcb9e431'
```
2. Second the single *layer* must be fetched:
```bash
curl -fsSL -D ./layer.head -o ./layer.body \
  -H 'Accept: application/vnd.docker.container.image.v1+json' \
  'https://registry-1.docker.io/v2/library/debian/blobs/67e8aa6c8bbc76b1f2bccb3864b0887671833b8667dc1f6c965fcb0eac7e6402'
```

## Building an image
The following `Dockerfile` is used as an example:

```Dockerfile
FROM debian:bullseye-slim
RUN echo 123 >/file
CMD ["date"]
```

It just adds a single dummy file to an official Debian image and configures a different default command.

Building this image with `docker build -t anatomy .` looks like this as of today (2022-05-20):

```
Sending build context to Docker daemon  2.048kB
Step 1/3 : FROM debian:bullseye-slim
bullseye-slim: Pulling from library/debian
214ca5fb9032: Pull complete
Digest: sha256:fbaacd55d14bd0ae0c0441c2347217da77ad83c517054623357d1f9d07f79f5e
Status: Downloaded newer image for debian:bullseye-slim
---> ed595c52174a
Step 2/3 : RUN echo 123 >file
---> Running in e74136713f11
Removing intermediate container e74136713f11
---> df8bf74c631b
Step 3/3 : CMD ["date"]
---> Running in f8f6371604fd
Removing intermediate container f8f6371604fd
---> 16d8a396acd5
Successfully built 16d8a396acd5
Successfully tagged anatomy:latest
```

Let's analyze those lines step by step in more detail:


1. ```
Sending build context to Docker daemon  2.048kB
```
   This copies the context, which in this example is the local directory `.`, to `dockerd`, which is orchestrates the build and runs as a system service as user *root*. The size to be transferred can be reduces by using a different directory or by excluding certain files and sub-directories via the file [.dockerignore](https://docs.docker.com/engine/reference/builder/#dockerignore-file).

2. ```
Step 1/3 : FROM debian:bullseye-slim
bullseye-slim: Pulling from library/debian
```
   While the `Dockerfile` specifies only the *repository tag* `debian:bullseye-slim` as the basis, `docker` defaults to `docker.io` and also prefixes the unclassified name (lacking any slash) automatically with `library/`. If the *tag* is not specified, it defaults to `latest`.

3. ```
214ca5fb9032: Pull complete
```
   That image consists of a single layer; the sha256 if that layer is `214ca5fb90323fe769c63a12af092f2572bf1c6b300263e09883909fc865d260`, which had to be pulled from the docker.hub registry as it was not cached locally.

4. ```
Digest: sha256:fbaacd55d14bd0ae0c0441c2347217da77ad83c517054623357d1f9d07f79f5e
```
   This is the *repository digest*. You can use `library/debian@sha256:fbaacd55d14bd0ae0c0441c2347217da77ad83c517054623357d1f9d07f79f5e` as a stable name for the image.

5. ```
Status: Downloaded newer image for debian:bullseye-slim
---> ed595c52174a
```
   This is the stable *image ID*; you can also use `ed595c52174af110ed61e8d65bd78923293dedabe035061bc3744078b90908f4` as a stable name for the image.

6. ```
Step 2/3 : RUN echo 123 >file
---> Running in e74136713f11
Removing intermediate container e74136713f11
---> df8bf74c631b
```
   This executes the 2nd line from `Dockerfile`. For this `dockerd` starts a new container based using the just downloaded Debian image `e74136713f11`; that hash is re-used as the *container name*. Inside the container the file `/file` is added to the root directory. As this command runs without showing any error, the intermediate container is stopped and removed, but the new modified file system content is saves as a new image `df8bf74c631b`.
    The the command fails, the intermediate container is not stopped and removed; this can be used to restart the container and debug the failure. Just remember to prune those containers after failues.

7. ```
Step 3/3 : CMD ["date"]
---> Running in f8f6371604fd
Removing intermediate container f8f6371604fd
---> 16d8a396acd5
```
   Just like above this executes the 3rd line from `Dockerfile`. As the file system content is not modified, no new layer is created, but the *configuration* is changes, which still creates a new manifest: it references the same 2 layers as the previous image, but a different configuration.

8. ```
Successfully built 16d8a396acd5Successfully tagged anatomy:latest
```
   This concludes the build and prints out the *image ID*. In addition to that it also associates a local *image tag* with that hash.

## Pushing an image
