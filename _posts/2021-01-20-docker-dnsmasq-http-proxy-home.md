---
title: 'docker, dnsmasq, http Proxy @ home'
date: '2021-01-20T07:59:31+01:00'
layout: post
categories: virt
---

I'm using docker on my notebook for development.
An mentioned in _UNIX 113: DocBook Docker images_ we already have several UCS images in docker-registry of many UCS releases and for different tasks:

- `phahn/minbase[:$major$minor$patch]`
  Minimal image with as few Debian binary packages pre-installed
- `phahn/aptbase[:$major$minor$patch]`
  minbase with UCS unmaintained enabled
- `phahn/devbase[:$major$minor$patch]`
  aptbase With Build-essentials tools pre-installed
- `phahn/debbase[:$major$minor$patch]`
  devbase With debhelper pre-installed
- `docbook`
  DocBook toolchain
- `docspell`
  Univention spell checker
- `ucs-ec2-tools`
  KVM and EC2 tools
- `univention-sphinx`
  Python Spinx documentation generator with Univention theme
- `knut/ssh`
  SSH client with SSH host keys of important KNUT hosts

he base images use `omar.knut.univention.de` and `updates.knut.univention.de` as their APT repository sources, so they only work when you're connected to KNUT.
But this may be not enough as you also need working DNS resolution and routing must work.

## dnsmasq

For me this was a problem as I use `dnsmasq` with `NetworkManager`, which provides two services:

1. It caches DNS queries.
2. It multiplexes DNS queries to the right server, e.g. requests for `.knut.univention.de` go to `nissedal` while all other queries go to via my local Internet router to my Internet provider.

For this `dnsmasq` is configured to bind to `localhost` only and thus my local `/etc/resolv.conf` file contains `server 127.0.0.1`.
But this breaks Docker which re-uses the content of that file to build the variant for its containers:
`127.0.0.1` references the **container**, not the **host**, so it would not work.
Therefor [Docker falls back](https://docs.docker.com/config/containers/container-networking/#dns-services) to using Googles servers `8.8.8.8` and `8.8.4.4`, but they cannot be used to resolve `.knut.univention.de`.

Running `dnsmasq` unbound on all interface does not work for me as I also use `libvirtd`, which also runs `dnsmasq` on its network bridges.
Binding the system `dnsmasq` to the `docker0` interface would work, but this is not supported by NetworkManager (AFAIK).

## Tinyproxy

Instead I choose to setup [Tinyproxy](http://tinyproxy.github.io/), a very small HTTP forwarding proxy.
It needed two small changes to its configuration file `/etc/tinyproxy/tinyproxy.conf` to bind it to my `docker0` bridge only and allow arbitrary uses of `CONNECT`:

```
Allow 172.17.0.0/12
#ConnectPort 443
#ConnectPort 563
```

## Docker

Next you have to tell the Docker client to [configure a proxy](https://docs.docker.com/network/proxy/#configure-the-docker-client) in `~/.docker/config.json`:
```json
{
 "proxies": {
  "default": {
   "httpProxy": "http://172.17.0.1:8888",
   "httpsProxy": "http://172.17.0.1:8888",
   "noProxy": "localhost,127.0.0.0/8"
  }
 }
}
```

Et voilà, now the images are able to phone home from within the docker container.

# Alternatives

Previously I always used `--dns 192.168.0.124 --dns-search knut.univention.de` or `--add-host updates.knut.univention.de:192.168.0.10 --add-host omar.knut.univention.de:192.168.0.10` when running a `docker` command.

Another alternative is to run the container with `docker run --network host …`.
In that case the container does not get its own separate network configuration, but uses the network configuration of the host when then also allows accessing KNUT via OpenVPN.

{% include abbreviations.md %}
