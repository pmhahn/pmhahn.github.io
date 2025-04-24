---
title: 'Gitlab 101: Container usage'
date: '2022-03-10T09:07:26+01:00'
layout: post
categories: virt git
---

Gitlab consists of multiple parts: Gitlab runners are the work horses, which execute the jobs of a pipeline. Gitlab runners have different [flavors](https://docs.gitlab.com/runner/executors/):

- [Shell](https://docs.gitlab.com/runner/executors/shell.html) and [ssh](https://docs.gitlab.com/runner/executors/shell.html) execute shell commands on a local or remote system as the logged in user. Only commands which are available to that user can be used which puts those runners in the category *kitten*: Someone has to setup and maintain them and you cannot easily scale them horizontally when you get into performance issues.
- [Parallels](https://docs.gitlab.com/runner/executors/parallels.html) and [VirtualBox](https://docs.gitlab.com/runner/executors/virtualbox.html) use virtual machines: Scaling is easier as they support templates, but the setup time and resource usage is still high, which makes them unattractive for running short lived commands.
- [Docker](https://docs.gitlab.com/runner/executors/docker.html) gives you the flexibility to choose any [OCI](https://opencontainers.org/ "Open Container Initiative") image for your job environment: You can use generic images and customize them as part of each job; or you can setup a second pipeline to [build a customized image](#building), which your original job then can just use. Quiet often the second variant pays off very quickly, especially if you have many jobs using the same image. For example we do this already for the DocBook and Sphinx based documentation build or the different images `ucs-{min,apt,dev,deb}base` used for building UCS, which incrementally go from minimal to `build-essential` and `debhelper` already being installed.
- [Docker machines](https://docs.gitlab.com/runner/executors/docker_machine.html) and [Kubernetes](https://docs.gitlab.com/runner/executors/kubernetes.html) (k8s) extend on Docker and add scalability on top: both allow setting up additional runners on demand and to down-scale again in idle time.

## Available runners

Runners can be shared by multiple projects, but you also can setup restricted runners which are only accessible by certain projects or even only to certain branches of those projects. Currently the following runners are available:

lattjo (docker, docker1, \*), sparka (docker, docker2, \*)
: Docker runners available to everyone
k8s (docker, k8s, \*)
: Experimental Kubernetes runner running on some old spare IBM server left over from a previous [OpenStack](https://www.openstack.org/) project and maintained by me
prod-dockermachine-s (dm-s, \*), prod-dockermachine-m (dm-m), prod-dockermachine-l (dm-l), prod-dockermachine-xl (dm-xl), prod-dockermachine-xxl (dm-xxl), prod-local (local)
: Docker-machine runners setup by #helpdesk with unknown capabilities
omar (ucs-build, omar), dimma (ucs-build, dimma)
: Shell runners to run build system related pipelines
~~upx-spoka-docker-runner~~(upx-docker), ansible-test (ansibleawxtestingdocker, awx, testing, docker)
: Test for build automation
(ulmer, special-build), ~~sahrens-docker-test<~~ (sahrens, docker, test, ansible)
: Special purpose or abandoned runners; do not use

If your job requires a specific runner, you can use the [tags:](https://docs.gitlab.com/ee/ci/yaml/#tags) tag to specify a **list of tags**, which an eligible runner must all satisfy: `tag: [docker, test]` is only be matched by *sahrens-docker-test* and the job can only run there.
Jobs with no tags are called *untagged* and are only processes by runners configured to do so.
Those runners are marked with `*` above.

## Using images

Using an image is quiet simple:
You simply add an [image:](https://docs.gitlab.com/ee/ci/yaml/#image) tag to your [.gitlab-ci.yml](https://docs.gitlab.com/ee/ci/yaml/) file and select the image you want to use.
An eligible runner will pull the image if necessary and the run the given [script:](https://docs.gitlab.com/ee/ci/yaml/#script) there.
Because of that you image **must not** have an [ENTRYPOINT](https://docs.docker.com/engine/reference/builder/#entrypoint) to run something other than a shell:
Gitlab expects the container to read the shell commands from STDIN and to send its output to STDOUR/STDERR.
Otherwise you must overwrite it using [image:entrypoint:](https://docs.gitlab.com/ee/ci/yaml/#imageentrypoint).

## Building images

Building a new OCI image on the other hand is much more complex:
Most people know and use [Docker](https://www.docker.com/), which is the most prominent [implementation of OCI](https://www.tutorialworks.com/difference-docker-containerd-runc-crio-oci/).
It consists of 3 parts:

containerd
: responsible for running and maintaining the containers and images
dockerd
: privileged daemon for docker specific services, providing the Docker API
docker
: command line to talk to `dockerd` via an UNIX socket `/run/docker.sock` or over an un-encrypted TCP connection to port 2375 or encrypted to port 2376.

When building a new image `dockerd` is doing the main work:
`docker build` simply instructs the daemon to do all the work, while the command line tool only provides access to local files and waits for the build to finish.
While `docker` can be run by anyone, `dockerd` and `containerd` require elevated privileges, which you normally do not want to hand out to anyone, as they can be used to escape from the container and to compromise the host system.
Some will argue that Docker is not for security, but if you're paranoid you should setup dedicated runners to run only sensitive pipelines or jobs.
Basically you have the following options:

1. Give all your docker containers `privileged = True`, so `dockerd` can be executed **inside** your regular build container. Easy to setup, but then all your containers can do other bad things.
2. Mount `/run/docker.sock` into your container and give `docker` access to `dockerd` **from the host**. Easy to setup, but then all your container can interfere with all other containers and the host.
3. You can run *Docker in Docker* (DinD) as a **side-car** container and only give that one elevated permissions. This is harder to setup, but gives each job its own `dockerd`, which adds to security and prevents cross-job interference. This is currently used by the runners on *lattjo*, *sparka* and *k8s* tagged with *docker*.

For using DinD you can re-use the following template definition using [extends: .docker](https://docs.gitlab.com/ee/ci/yaml/#extends):

```yaml
.docker:
  tags:
    - docker
  services:
    - name: docker-registry.knut.univention.de/ucs/docker:dind
      alias: docker
  variables:
    DOCKER_HOST: tcp://docker:2375/
    DOCKER_TLS_VERIFY: ""
```

- `DOCKR_HOST` tells `docker` to communicate with its `dockerd` over the given TCP connection.
- `services:` creates that side-car container running our customized version of `dockerd`, which is accessible via the DNS name `docker`.
- `name:` specifies an internal image, which already has the [KNUT root CA certificate](#storing) pre-installed and has [registry mirroring](#caching) setup correctly for our internal infrastructure.
- `tags:` restricts the job to running only one the hosts supporting DinD.
- For now `DOCKER_TLS_VERIFY` disables using TLS between `docker` and `dockerd`. Gitlab guarantees that both containers run on the same host and that this is only a local connection between two chummy containers. TLS can be added, but requires much more work to pass the uniquely generated certificates from the `dockerd` container to build container, where `docker` then expects them to find.

There are alternatives to Docker:

- [Podman](https://podman.io/) does not have a privileged daemon and the command line tool does every thing itself. I have not investigated this further.
- [Kaniko](https://github.com/GoogleContainerTools/kaniko) is another alternative, which does not require elevated privileges at all. Using it is [documented by Gitlab](https://docs.gitlab.com/ee/ci/docker/using_kaniko.html) and Sönke has successfully used to to build _GoBBB_.

## Caching images

Using DinD has one problem:
For each job you get a new empty `dockerd`.
This also applies to VM based Gitlab Runners, where VMs are cloned and trashed regularly.
Every build or run has to pull the required images first.
Since [DockerHub rate limiting](https://about.gitlab.com/blog/2020/10/30/mitigating-the-impact-of-docker-hub-pull-requests-limits/) this easily becomes a bottleneck:
Not only are (large) images pulled multiple times taking bandwidth, but you're only allowed to do 100 requests per 6 hours.
Therefore it is essential to use *Registry mirroring* and start `dockerd` with `--registry-mirrors http://docker-registry.knut.univention.de:5001`.
With this you can continue using images like `debian:buster-slim` from [DockerHub](https://hub.docker.com/), which are then cached internally.
As an alternative you can adapt the image names and pull from some other Registry, namely:

- We pull several often used images on a weekly basis and push them into our internal registry `docker-registry.knut.univention.de/debian:buster-slim`.
- Gitlab also has a built-in [Dependency Proxy](https://docs.gitlab.com/ee/user/packages/dependency_proxy/): `${CI_DEPENDENCY_PROXY_GROUP_IMAGE_PREFIX}/debian:buster-slim` uses the Proxy of the top-level group (e.g. `univention`) while `${CI_DEPENDENCY_PROXY_DIRECT_GROUP_IMAGE_PREFIX}/debian:buster-slim` uses the Proxy of the sub-group (e.g. `univention/dist`). This is easy when using a image in a pipeline, but requires using `docker build --build-arg …` when building images to pass through those variables.
- You can pull from our internal Harbor registry setup for Phönix, which has a transparent proxy for [DockerHub](https://hub.docker.com/) and [Quay.io](https://quay.io/): `docker pull artifacts.knut.univention.de/dockerhub_proxy_cache/library/debian:buster-slim`. But see below for issues.

## Storing images

A newly built image is only available to the local `containerd` first unless it is tagged and pushed to some other registry.

- `docker-registry.knut.univention.de` is a [plain docker registry](https://docs.docker.com/registry/), which can be used **internally by everyone**: it offers both encrypted and un-encrypted access, requires no authentication, has no GUI, has no expiration mechanism and fills up the storage backend chronically. It is badly (un-)maintained.
- `artifacts.knut` is said [Harbor registry](https://goharbor.io/), which is encrypted, requires authentication, has a GUI, has built-in security scanning, and several other features. It was setup for the Phönix project by Ferenç, who has since left Univention in 2022. As such the state of our instance is currently unknown respective unmaintained. While authentication is good, this requires extra work in Gitlab pipelines as there is no built-in mechanism for automatic authentication.
- In Gitlab every project has its own [Container registry](https://docs.gitlab.com/ee/user/packages/container_registry/), which can be used to store the images created from that project. `gitregistry.knut.univention.de` is encrypted, has authentication built-in with pipelines, has a GUI, has an expiration mechanism and security scanning can be included. Previously it was not to be used because of lacking storage space, but with the move of Gitlab to Plusserver we now have a S3 compatible object store, which hopefully will scale with us.

The biggest issue right now is **encryption**:
All parts of Gitlab like `git`, `gitregistry`, `gitpages` but also `artifacts` use TLS encryption by default and use a certificate from **KNUT**, which **nobody** knows by default:
Every time you use a standard OCI image you have to manually insert our root certificate, mostly copy it to `/usr/local/share/ca-certificates/` and call `update-ca-certificates`.
Depending on the container you may have to do different things:
For [Alpine Linux](https://alpinelinux.org/) you have to use `apk add --no-cache ca-certificates`, for [Debian](https://debian.org/) based images you have to to `apt-get -qq update && apt-get -y --assume-yes install ca-certificates`.
Instead of downloading the certificate each time using `wget` or `curl` (which also may have to be installed separately) our Gitlab instance has a [File type CI/CD variable](https://docs.gitlab.com/ee/ci/variables/#cicd-variable-types) named `KNUT_CA`, which points to a file injected into every pipeline containing the root certificate.
Thus it can be installed using `install -m 644 -D "$KNUT_CA" /usr/local/share/ca-certificates/knut-ca.crt` or similar.

While this sound easy in theory, in reality it is a major pain as this must be done for nearly every container.
For example this prevents the use of [Auto DevOps](https://docs.gitlab.com/ee/topics/autodevops/), which internally uses lots of helper containers for doing its magic.
Every use breaks as soon as any container tries to access out infrastructure secured by a KNUT certificate.

Sönke and I already talked about this for some time and we have the idea to acquire certificates from [Let's encrypt](https://letsencrypt.org/de/) for those **internal** systems, which would greatly simplify the usage.
For now I can not yet recommend using the Gitlab registry, because — simply spoken — there are too many pitfalls and requires deep technical knowledge to use it.

{% include abbreviations.md %}
