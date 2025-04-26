---
title: 'Docker 101: Container basics'
date: '2022-06-08T11:33:56+02:00'
layout: post
categories: container
---

Following [Gitlab 101: Container usage]({% post_url 2022-03-10-gitlab-101-container-usage %}) and [Gitlab 102: container followup]({% post_url 2022-05-24-gitlab-102-container-followup %}) let's have a look at *Docker images* itself, also known as [OCI](https://www.opencontainers.org/ "Open Container Initative") Images.
If you want to know the gory details have a look at the [image specification](https://github.com/opencontainers/image-spec/blob/main/spec.md) yourself.

When working with images multiple parts are involved:

1. A number of **layers** containing the files of the image
2. A **configuration** containing instructions to setup the environment for an image
3. A **manifest** to describe an image
4. A **directory** to map human usable names to sha256s

The last two only exist when the image is uploaded to respectively downloaded from a (remote) registry:
When images are transferred between hosts the layers are transferred as *compressed tape archives* (`.tar.gz`).
Locally they are stored in some other (unpacked) format as `.tgz` files are not efficient for random access.

Content like manifests, configurations and layers are identified by their unique sha256 fingerprint.
Any change would result in a different sha256 unless you get a [collision](https://en.wikipedia.org/wiki/Collision_resistance), which is very unlikely to happen.
(Docker changed their implementation some years ago to using the sha256 instead of some random [UUID](https://en.wikipedia.org/wiki/Universally_unique_identifier "Universally Unique Identifier").)

## A simple example

This example creates a minimal image with just one file inside:
```console
# mkdir temp
# printf 'FROM scratch\nADD Dockerfile /\n' >temp/Dockerfile

# docker build -t empty temp/
Sending build context to Docker daemon  2.048kB
Step 1/2 : FROM scratch
 --->
Step 2/2 : ADD Dockerfile /
 ---> 4cd7d90bcd38
Successfully built 4cd7d90bcd38
Successfully tagged empty:latest

# docker image ls –digests –all –no-trunc
REPOSITORY   TAG      DIGEST   IMAGE ID               CREATED          SIZE
empty        latest            sha256:4cd7d90bcd38…   11 seconds ago   30B
```

- REPOSITORY:empty
  (local) name of the image.
- TAG:latest
  (local) tag for the image;
  it defaults to `latest`.
- DIGEST:<none>
  (global) sha256 of manifest;
  exists only when image is `push`ed or `pull`ed.
- IMAGE ID:4cd7d90bcd3800618661bc93f9a8d298a2dcffa7bf179a990c6be9f0b3da3a60
  (local) sha256 of configuration.
- CREATED:11 seconds ago
  Time image was created.
- SIZE:30B
  Cumulative size of all layers.

## Docker registry layout

### Directory

At the top you have a **directory** like [Docker Hub](https://hub.docker.com/) or [Quay.io](https://quay.io/) or a [Gitlab container registry](https://docs.gitlab.com/ee/user/packages/container_registry/) or a [private docker registry](https://docs.docker.com/registry/) like `docker-registry.knut` or [docker.software-univention.de](https://docker.software-univention.de/v2/_catalog).
They map **image names and tags** to an image, which is known by its sha256 hash.
This mapping is **volatile** and can be updated any time.
If you want [reproducible build](https://reproducible-builds.org/) you should do this lookup once manually and then specify the sha256 instead of the name.

### Configuration

The configuration describes the image locally, listing all its layers, environment variables, pre-configured commands, and the history, how the image was created.
Docker stores it locally below `/var/lib/docker/image/overlay2/imagedb/content/sha256/` using the sha256 as the filename.

<details><summary>Example configuration</summary>

`/var/lib/docker/image/overlay2/imagedb/content/sha256/4cd7d90bcd3800618661bc93f9a8d298a2dcffa7bf179a990c6be9f0b3da3a60`

```json
{
 "architecture": "amd64",
 "config": {
  "Hostname": "",
  "Domainname": "",
  "User": "",
  "AttachStdin": false,
  "AttachStdout": false,
  "AttachStderr": false,
  "Tty": false,
  "OpenStdin": false,
  "StdinOnce": false,
  "Env": [
   "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
  ],
  "Cmd": null,
  "Image": "",
  "Volumes": null,
  "WorkingDir": "",
  "Entrypoint": null,
  "OnBuild": null,
  "Labels": null
 },
 "container_config": {
  "Hostname": "",
  "Domainname": "",
  "User": "",
  "AttachStdin": false,
  "AttachStdout": false,
  "AttachStderr": false,
  "Tty": false,
  "OpenStdin": false,
  "StdinOnce": false,
  "Env": [
   "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
  ],
  "Cmd": [
   "/bin/sh",
   "-c",
   "#(nop) ADD file:828c92fea7a1430c77481255062a481688cda9d60dd8c1caac9c8b3cfb1c33a5 in / "
  ],
  "Image": "",
  "Volumes": null,
  "WorkingDir": "",
  "Entrypoint": null,
  "OnBuild": null,
  "Labels": null
 },
 "created": "2022-06-08T05:54:02.779544126Z",
 "docker_version": "19.03.8",
 "history": [
  {
   "created": "2022-06-08T05:54:02.779544126Z",
   "created_by": "/bin/sh -c #(nop) ADD file:828c92fea7a1430c77481255062a481688cda9d60dd8c1caac9c8b3cfb1c33a5 in / "
  }
 ],
 "os": "linux",
 "rootfs": {
  "type": "layers",
  "diff_ids": [
   "sha256:9d1e415268eb1a4f5ac75d7f42983be65f92d94d0d5f5648bb1489acaa35e329"
  ]
 }
}
```
Additional metadata is stored in a second location below `/var/lib/docker/image/overlay2/imagedb/metadata/sha256/` containing the timestamp of last update.

</details>

At the end it lists the layers of the image:
```json
 "rootfs": {
  "type": "layers",
  "diff_ids": [
   "sha256:9d1e415268eb1a4f5ac75d7f42983be65f92d94d0d5f5648bb1489acaa35e329"
  ]
}
```
They are stored below `/var/lib/docker/image/overlay2/layerdb/sha256/` in some compressed JSON format.
Before it can be used it will get unpacked to `/var/lib/docker/overlay2/`.

### Manifests

An image is described by a **manifest**, which names the *architecture* the image is for, references a *configuration* document by sha256 and all the stacked *layers* by their hash required to assemble the complete image.
Multiple architectures can be supported by building the image for each architecture and pushing them to the same name.
This creates an additional manifest for each architecture and also a **manifest-list**, which references all of them and itself is pointed to in the directory.

```console
# docker run -d -p 5000:5000 –restart always –name registry registry:2
# docker tag empty:latest localhost:5000/empty:latest
# docker push localhost:5000/empty:latest
The push refers to repository [localhost:5000/empty]
9d1e415268eb: Pushed
latest: digest: sha256:4cb519a68a7d30937115b054a02e82df0d44477d418dbf81a74472975dfb70e2 size: 524
```

Note that while (in this case) the *layer ID* `9d1e415268eb` matches the previously seen value, the *manifest ID* `4c**b519a68a…**` is different from the *image ID* `4c**d7d90bcd38…**`!
This can be confirmed by running the following command again:

```console
# docker image ls –digests –no-trunc
REPOSITORY             TAG                 DIGEST                                                                    IMAGE ID                                                                  CREATED             SIZE
empty                  latest                                                                                        sha256:4cd7d90bcd3800618661bc93f9a8d298a2dcffa7bf179a990c6be9f0b3da3a60   2 hours ago         30B
localhost:5000/empty   latest              sha256:4cb519a68a7d30937115b054a02e82df0d44477d418dbf81a74472975dfb70e2   sha256:4cd7d90bcd3800618661bc93f9a8d298a2dcffa7bf179a990c6be9f0b3da3a60   2 hours ago         30B
registry               2                   sha256:bedef0f1d248508fe0a16d2cacea1d2e68e899b2220e2258f1b604e1f327d475   sha256:773dbf02e42e2691c752b74e9b7745623c4279e4eeefe734804a32695e46e2f3   12 days ago         24.1MB
```

Docker keeps track of this mapping in the file `/var/lib/docker/image/overlay2/repositories.json`, which maps global manifest IDs to local image IDs.

<details><summary>Example manifest</summary>

The manifest can be fetched from the registry by running `curl http://localhost:5000/v2/empty/manifests/latest`, which returns this:

```json
{
 "schemaVersion": 1,
 "name": "empty",
 "tag": "latest",
 "architecture": "amd64",
 "fsLayers": [
  {
   "blobSum": "sha256:baf0be5522d4916aa5f49282fee717c0db36dd76bf3596ee1a581e364618d372"
  }
 ],
 "history": [
  {
   "v1Compatibility": "{\"architecture\":\"amd64\",\"config\":{\"Hostname\":\"\",\"Domainname\":\"\",\"User\":\"\",\"AttachStdin\":false,\"AttachStdout\":false,\"AttachStderr\":false,\"Tty\":false,\"OpenStdin\":false,\"StdinOnce\":false,\"Env\":\[\"PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin\"\],\"Cmd\":null,\"Image\":\"\",\"Volumes\":null,\"WorkingDir\":\"\",\"Entrypoint\":null,\"OnBuild\":null,\"Labels\":null},\"container_config\":{\"Hostname\":\"\",\"Domainname\":\"\",\"User\":\"\",\"AttachStdin\":false,\"AttachStdout\":false,\"AttachStderr\":false,\"Tty\":false,\"OpenStdin\":false,\"StdinOnce\":false,\"Env\":\[\"PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin\"\],\"Cmd\":\[\"/bin/sh\",\"-c\",\"#(nop) ADD file:828c92fea7a1430c77481255062a481688cda9d60dd8c1caac9c8b3cfb1c33a5 in / \"\],\"Image\":\"\",\"Volumes\":null,\"WorkingDir\":\"\",\"Entrypoint\":null,\"OnBuild\":null,\"Labels\":null},\"created\":\"2022-06-08T05:54:02.779544126Z\",\"docker_version\":\"19.03.8\",\"id\":\"af57927a060f5d8c6c82198d7069f4e21c9de36a531df0938b43e4fa34fb1824\",\"os\":\"linux\"}"
  }
 ],
 "signatures": [
  {
   "header": {
    "jwk": {
     "crv": "P-256",
     "kid": "PR3Z:67EI:UWBM:I3FK:6KGU:QDZF:3HPS:5W2Q:PLQY:A3RI:DOGW:NWH2",
     "kty": "EC",
     "x": "7ucvD4agvS9KIAzY4FXIOcnnghueywtyhMTK06Kmf9Y",
     "y": "aeBOOkFSi45NKQ7EbCrgC-NTrncQaIDoiP5xk8FH8_s"
    },
    "alg": "ES256"
   },
   "signature": "fXsMf6lmrMUktaaiElX6Udinhdx-s9Ub_TQ9NHIjKwcLyqq9XA6wSPYcJzaLJAxJBoEENpW0AoPkqraWvQIq6A",
   "protected": "eyJmb3JtYXRMZW5ndGgiOjEzNzksImZvcm1hdFRhaWwiOiJDbjAiLCJ0aW1lIjoiMjAyMi0wNi0wOFQwNzo1OToxOVoifQ"
  }
 ]
}
```

A readable version of the configuration can be produced by the following command, which equals the configuration from above:

```bash
curl http://localhost:5000/v2/empty/manifests/latest |
 jq -r .history[0].v1Compatibility |
 jq .
```

</details>

In the middle is the list of layers:
```json
 "fsLayers": [
  {
   "blobSum": "sha256:baf0be5522d4916aa5f49282fee717c0db36dd76bf3596ee1a581e364618d372"
  }
 ],
```

### Layers

Each **layer** consists of a set of files, which are additive by nature.
They have to be stacked over each other in the right order to assemble the complete file system.
Each layer is read-only to prevent any kind of modification.
A writable layer is put on top last to allow multiple containers to use the same image without interfering with each other.
Changing an existing file will trigger a *pull-up*, where the original file is copied from its read-only-layer to the writable top-layer first before the write occurs.
Deleting a file will simply create a *white-out* entry in the top-layer, which will hide the underlying file, so this actually will not free any space.

As stated above layers are transferred as compressed TAR files, which can be accessed directly like this:
```console
# curl –silent –fail –show-error http://localhost:5000/v2/empty/blobs/sha256:baf0be5522d4916aa5f49282fee717c0db36dd76bf3596ee1a581e364618d372 |
 gzip -dc | tar -t -f – -v
-rw-r--r-- 0/0              30 2022-06-08 07:54 Dockerfile
```

`baf0be5522d4916aa5f49282fee717c0db36dd76bf3596ee1a581e364618d372` from the manifests equals the sha256 of the **compressed** archive.
After importing it the local configuration lists has *diff\_id* `9d1e415268eb1a4f5ac75d7f42983be65f92d94d0d5f5648bb1489acaa35e329`, which equals the shas256 of the **uncompressed** archive:
```console
# curl –silent –fail –show-error http://localhost:5000/v2/empty/blobs/sha256:baf0be5522d4916aa5f49282fee717c0db36dd76bf3596ee1a581e364618d372 |
 gzip -dc | sha256sum
9d1e415268eb1a4f5ac75d7f42983be65f92d94d0d5f5648bb1489acaa35e329 –
```

## Summary

Docker uses a *content addressable* scheme to store its data.
It uses *stacked layers* to efficiently handle images.
While layers may be shared by multiple images, their *configurations* will be different, leading to different sha256 fingerprints.
Depending on if an image was pushed or pulled a *manifest* may be involved, which add yet another sha256.

If you're still not confused, running containers get yet another sha256 ID and docker uses some completely different identifiers internally below `/var/lib/docker/overlay2/l/`.

## Appendix

Linux had many stacking file systems like [UnionFS](https://en.wikipedia.org/wiki/UnionFS), Translucency, Ovlfs, Mini\_fo, Mula-FS, [AUFS](https://en.wikipedia.org/wiki/Aufs), Cowloop in the past, but [OverlayFS](https://en.wikipedia.org/wiki/OverlayFS) got merged in 2014 and is the default since than.

A followup blog post will take a deeper look at the docker registry itself, which will also include adding authorization.

## Links

- [Explaining Docker Image IDs](https://windsock.io/explaining-docker-image-ids/)
- [Dockerfile reference](https://docs.docker.com/engine/reference/builder/#from)
- [Docker Registry API](https://docs.docker.com/registry/spec/api/)

{% include abbreviations.md %}
