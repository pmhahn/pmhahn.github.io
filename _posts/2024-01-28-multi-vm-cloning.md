---
layout: post
title: "Cloning multiple virtual machines"
date: 2024-01-28 16:20:00  +0100
categories: network linux virt
excerpt_separator: <!--more-->
---

Problem
=======

I want to clone **multiple** virtual machines belonging together.
They internally communicate among themselves using their **static** IP addresses.
Changing the network configuration per clone is not an option.
On the other hand I want to connect to these machines from the **outside**.

```
┌──────────────────────host─────────────────────┐
│ ┌───────────env1────────────┐                 │
│ │ ┌───vm1.2──┐ ┌───vm1.3──┐ │                 │
│ │ │ 10.0.0.2 │ │ 10.0.0.3 │ ├──┐              │
│ │ └──────────┘ └──────────┘ │  │              │
│ └───────────────────────────┘  │              │
│                                │              │
│ ┌───────────env2────────────┐  │              │
│ │ ┌───vm2.2──┐ ┌───vm2.3──┐ │  ├───────┐      │
│ │ │ 10.0.0.2 │ │ 10.0.0.3 │ ├──┤ magic ├─eth0─┼─
│ │ └──────────┘ └──────────┘ │  ├───────┘      │
│ └───────────────────────────┘  │              │
│                                │              │
│ ┌───────────env3────────────┐  │              │
│ │ ...                       ├──┘              │
│ └───────────────────────────┘                 │
└───────────────────────────────────────────────┘
```

<!--more-->

Each *environmet* is mapped to a separate *bridge*.
This allows the VMs inside each bridge to use the *same* IP addresses.

To allow inbound and outbound connections a router is needed, which connects each bridge to the host.
Network address translation is needed for both incoming and outgoing packages.
The address of that gateway `10.0.0.1` will also be static per environment.
Now we have **multiple** interfaces sharing the **same** IP address.
To solve this issue we have to use *network name spaces* to create a *virtual router* for each environment / bridge.
Each router is then connected to the outside world either via a *host bridge* or my using MACVLAN interfaces.

The revised network setup looks like this.

```
┌────────────────────────────────────────host──────────────────────────────────┐
│ ┌───vm1.2──┐       ┌─br1─┐                                      ┌─br0─┐      │
│ │ 10.0.0.2 ├─tap1──┤     │       ┌──────────ns1─────────┐       │     │      │
│ └──────────┘       │     │       │                      │       │     │      │
│                    │     ├─veth1─┤ 10.0.0.1  172.16.1.1 ├─veth2─┤     │      │
│ ┌───vm1.3──┐       │     │       │                      │       │     │      │
│ │ 10.0.0.3 ├─tap2──┤     │       └──────────────────────┘       │     │      │
│ └──────────┘       └─────┘                                      │     │      │
│                                                                 │     ├─eth0─┼─
│ ┌───vm2.2──┐       ┌─br2─┐                                      │     │      │
│ │ 10.0.0.2 ├─tap3──┤     │       ┌──────────ns2─────────┐       │     │      │
│ └──────────┘       │     │       │                      │       │     │      │
│                    │     ├─veth3─┤ 10.0.0.1  172.16.2.1 ├─veth4─┤     │      │
│ ┌───vm2.3──┐       │     │       │                      │       │     │      │
│ │ 10.0.0.3 ├─tap4──┤     │       └──────────────────────┘       │     │      │
│ └──────────┘       └─────┘                                      └─────┘      │
└──────────────────────────────────────────────────────────────────────────────┘
```

This can be simplified by using MACVLAN:
In contrast to *bridging* is does not need to learn MAC addresses as all (internal) MAC addresses are always known.
The downside is that it cannot be used for host-to-VM communication directly.

```
┌────────────────────────────────────host───────────────────────────────────┐
│ ┌───vm1.2──┐       ┌─br1─┐                                                │
│ │ 10.0.0.2 ├─tap1──┤     │       ┌──────────ns1─────────┐                 │
│ └──────────┘       │     │       │                      │                 │
│                    │     ├─veth1─┤ 10.0.0.1  172.16.1.1 ├─macvlan1─┐      │
│ ┌───vm1.3──┐       │     │       │                      │          │      │
│ │ 10.0.0.3 ├─tap2──┤     │       └──────────────────────┘          │      │
│ └──────────┘       └─────┘                                         │      │
│                                                                    ├─eth0─┼─
│ ┌───vm2.2──┐       ┌─br2─┐                                         │      │
│ │ 10.0.0.2 ├─tap3──┤     │       ┌──────────ns2─────────┐          │      │
│ └──────────┘       │     │       │                      │          │      │
│                    │     ├─veth3─┤ 10.0.0.1  172.16.2.1 ├─macvlan2─┘      │
│ ┌───vm2.3──┐       │     │       │                      │                 │
│ │ 10.0.0.3 ├─tap4──┤     │       └──────────────────────┘                 │
│ └──────────┘       └─────┘                                                │
└───────────────────────────────────────────────────────────────────────────┘
```

On top of that I also added *network address translation* (NAT) to make those VMs accessible from the outside.
I map each VM to an IP address from the private network address range `172.16.$ENV.$VM/16`.

Doing
=====

```sh
#!/bin/sh
set -e -u -x

: "${USER:=pmhahn}" "${GROUP:=pmhahn}"
: "${DEV:=enp34s0}"  # host network interface

# Enable IP forwarding and Proxy ARP
echo 1 >'/proc/sys/net/ipv4/ip_forward'
echo 1 >'/proc/sys/net/ipv4/conf/all/proxy_arp'
echo 1 >"/proc/sys/net/ipv4/conf/$DEV/proxy_arp"
echo 1 >'/proc/sys/net/ipv4/conf/default/proxy_arp'

for ENV in 1 2 # ...
do
  # Create bridge
  ip link add name "br$ENV" type bridge
  ip link set dev "br$ENV" up

  # Setup router
  ip netns add "ns$ENV"

  # Connect router to host network
  ip link add name "gw$ENV" link "$DEV" type macvlan
  ip link set dev "gw$ENV" netns "ns$ENV"
  ip netns exec "ns$ENV" ip addr add "172.16.$ENV.1/16" dev "gw$ENV"
  ip netns exec "ns$ENV" ip link set dev "gw$ENV" up
  ip netns exec "ns$ENV" ip route add default via 172.16.0.1

  # Connect bridge to router
  ip link add "peer1-br$ENV" type veth peer name "peer1-gw$ENV"
  ip link set "peer1-br$ENV" master "br$ENV"
  ip link set "peer1-br$ENV" up
  ip link set dev "peer1-gw$ENV" netns "ns$ENV"
  ip netns exec "ns$ENV" ip addr add 10.0.0.1/24 dev "peer1-gw$ENV"
  ip netns exec "ns$ENV" ip link set dev "peer1-gw$ENV" up
  ip netns exec "ns$ENV" ip route add "172.16.$ENV.0/24" dev "peer1-gw$ENV"

  # Setup NAT for each VM
  for VM in 2 3 # ...
  do
    ip netns exec "ns$ENV" iptables -t nat -A PREROUTING  -d "172.16.$ENV.$VM" -j DNAT --to-destination "10.0.0.$VM"
    ip netns exec "ns$ENV" iptables -t nat -A POSTROUTING -s "10.0.0.$VM"      -j SNAT --to             "172.16.$ENV.$VM"

    ip tuntap add dev "vm$ENV.$VM" mode tap user "$USER" group "$GROUP"
    ip link set "vm$ENV.$VM" master "br$ENV"
    ip link set "vm$ENV.$VM" up
  done
done

tcpdump -i br1 net 172.16.0.0/16

exit 0
```

Testing
=======

Instead of running full VMs I decided to create a *minimal Initial RAM disk*.
Is is easily done using [Dnaiel P. Berrangé: make-tiny-image.py](https://www.berrange.com/posts/2023/03/09/make-tiny-image-py-creating-tiny-initrds-for-testing-qemu-or-linux-kernel-userspace-behaviour/).

```sh
# Create minimal InitRamFS for testing with QEMU
PATH=$PATH:/sbin:/usr/sbin ./make-tiny-image.py --kmod e1000 \

# Start a VM
qemu-system-x86_64 \
  -kernel "/boot/vmlinuz-$(uname -r)" \
  -initrd ./tiny-initrd.img \
  -append 'console=ttyS0 quiet' \
  -accel kvm \
  -m 1024 \
  -display none \
  -serial stdio \
  -netdev tap,id=net,ifname="vm$ENV.$VM",script=no,downscript=no -device virtio,netdev=net

# Inside the VM setup the network:
ip addr add "10.0.0.$VM/24" dev eth0
ip link set eth0 up
ip route add default via 10.0.0.1"
ping -c 10.0.0.1
```

Open issues
===========

1. For some strange reason the VM was first unreachable.
   Only after doing a `ping 10.0.0.1` from inside the VM everything started to work.

Background
==========

I work for [Univention GmbH](https://www.univention.com/).
Our main product is [Univention Corporate Server](https://www.univention.com/products/ucs/) (UCS).
It is an enterprise operating system based on [Debian GNU/Linux](https://www.debian.org/).
For availability and for scaling a standard setup consists of multiple hosts connected via a network.
Our current test setup always starts with a fresh installation:
The *Debian Installer* already has partitioned the disks and installed basic packages, but *provisioning* has not happened yet.
As each server role installs a different set of packages, this is re-done for each test setup.
That takes a long time as many packages are installed and many scripts are run.
This takes extra long for our *upgrade tests*, which setup an old version of UCS and then install all available updates.

Running the full test suite takes 7 hours 30 minutes to 8 hours 30 minutes.
Setting up the environment takes 40 minutes to 1 hour 20 minutes.
Upgrade tests take even longer and take up to 13 hours:
We start with UCS 4.4-9, and then upgrade to 5.0-0 to 5.0-1 to 5.0-2 to 5.0-3 to 5.0-4 to 5.0-5 to finally 5.0-6.
This is the worst scenario but we still need it as UCS 4.4-9 is still maintained:
We have to guarantee that our customers still running 4.4-9 will be able to upgrade to 5.0-6 and that their setup will still work then.

Closing notes
=============
- Stateless NAT (`ip route add net`) is deprecated and has been removed with Linux [2.6.24](https://linux.die.net/man/8/ip).

Links
=====

* [Chris: Using network namespaces with veth to NAT guests with overlapping IPs](https://blog.christophersmart.com/2020/03/15/using-network-namespaces-with-veth-to-nat-guests-with-overlapping-ips/)
* [Debian Wiki: Proxy ARP](https://wiki.debian.org/BridgeNetworkConnectionsProxyArp)
* [Linux IP: Stateless NAT with iproute2](http://linux-ip.net/html/nat-stateless.html)
* [StackExchange: Stateless NAT](https://unix.stackexchange.com/questions/559877/how-can-a-global-scope-be-invalid-when-creating-a-route-with-the-ip-command) (deprecated since 2.6.24)
* [Dnaiel P. Berrangé: make-tiny-image.py](https://www.berrange.com/posts/2023/03/09/make-tiny-image-py-creating-tiny-initrds-for-testing-qemu-or-linux-kernel-userspace-behaviour/)
* [QEMU VM templating](https://qemu-project.gitlab.io/qemu/system/vm-templating.html)

{% include abbreviations.md %}
