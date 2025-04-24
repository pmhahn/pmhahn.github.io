---
layout: post
title: "Ceph-Jewel RBD libvirt storage pool"
date: 2019-11-09 11:03:00  +0200
categories: linux UCS filesystem virt
excerpt_separator: <!--more-->
---

For some development work on an [Univention Corporate Server
4.4](https://www.univention.de/blog-de/2019/10/verteilter-datenspeicher-mit-ucs-und-ceph/),
which is based on Debian Stretch, I needed a [Ceph cluster](https://ceph.io/)
based on the Jewel release. Most of the tutorials were based on newer [Ceph
releases](https://docs.ceph.com/docs/master/releases/) (Luminous, Mimic) or
were using [ceph-deploy](https://docs.ceph.com/docs/master/start/), which is
not part of Debian and must be installed separately.

Therefor I did a [manual
installation](https://docs.ceph.com/docs/jewel/install/manual-deployment/),
using the low-level tools. In contrast to [Ceph Storage with
UCS](https://help.univention.com/t/cool-solution-ceph-storage-with-ucs/12535)
I do not want to use CephFS, but use Rados directly.

This is a **test setup** and not appropriate for production:
* I'm only running one monitor, which is a *single point of failure*.
* I disabled replication on purpose.
* all nodes have a single big `ext4` file system, so no partitioning, no fast
  journal disks, and [issues with extended attributes](https://docs.ceph.com/docs/jewel/rados/configuration/filesystem-recommendations/#not-recommended).

<!--more-->

My setup consists of 3 hosts, which are in the `192.168.0.0/24` network and use my domain `phahn.dev`:

* hdmi1 = 192.168.0.15 = `ceph-mon`, `libvirtd`
* hdmi2 = 192.168.0.18 = `ceph-osd`
* hdmi3 = 192.168.0.24 = `ceph-osd`

Commands are executed on `hdmi1`, where I'm logged in as the user `root`:

# Setup SSH for password less login

```bash
ssh-keygen -N '' -t rsa -f /root/.ssh/id_rsa
ssh-copy-id hdmi1
ssh-copy-id hdmi2
ssh-copy-id hdmi3
```

# Setup firewall

Port 6789 is used for inter-OSD-communication, port 6800-7100 for clients to
connect to the OSD.

```bash
for osd in hdmi1 hdmi2 hdmi3
do
  ssh "$osd" "ucr set security/packetfilter/tcp/6789/all=ACCEPT"
  ssh "$osd" "ucr set security/packetfilter/tcp/6800:7100/all=ACCEPT"
  ssh "$osd" "systemctl restart univention-firewall.service"
done
```

# Create initial `ceph.conf`

Following
[ceph.conf](//docs.ceph.com/docs/jewel/rados/configuration/ceph-conf/) I
create a *degraded* cluster:

* My nodes only have a single network interface, so my public network is the
  same as my internal (`cluser`) network.
* You have to explicitly list all OSD with their name; otherwise the monitor
  will not initialize the OSDs correctly and they will remain stuck in the
  `booting` stage.

```bash
FS_UUID=$(uuidgen)
cat >/etc/ceph/ceph.conf <<__CONF__
[global]
fsid = $FS_UUID
public network = 192.168.0.0/24
cluster network = 192.168.0.0/24

auth cluster required = cephx
auth service required = cephx
auth client required = cephx

osd pool default size = 1
osd pool default min size = 1
osd pool default pg num = 200
osd pool default pgp num = 200
osd crush chooseleaf type = 0

[mon]
mon initial members = hdmi1
mon host = 192.168.0.15

[osd]
osd journal size = 1024
# For ext4:
osd max object name len = 256
osd max object namespace len = 64

[osd.0]
host = hdmi2

[osd.1]
host = hdmi3

[client]
rbd cache                           = true
__CONF__
scp /etc/ceph/ceph.conf hdmi2:/etc/ceph/ceph.conf
scp /etc/ceph/ceph.conf hdmi3:/etc/ceph/ceph.conf
```

# Setup Ceph monitor

```bash
ceph-authtool --create-keyring /tmp/ceph.mon.keyring --gen-key -n mon. --cap mon 'allow *'

sudo ceph-authtool --create-keyring /etc/ceph/ceph.client.admin.keyring --gen-key -n client.admin --cap mon 'allow *' --cap osd 'allow *' --cap mds 'allow *' --cap mgr 'allow *'

sudo ceph-authtool --create-keyring /var/lib/ceph/bootstrap-osd/ceph.keyring --gen-key -n client.bootstrap-osd --cap mon 'profile bootstrap-osd' --cap mgr 'allow r'

sudo ceph-authtool /tmp/ceph.mon.keyring --import-keyring /etc/ceph/ceph.client.admin.keyring
sudo ceph-authtool /tmp/ceph.mon.keyring --import-keyring /var/lib/ceph/bootstrap-osd/ceph.keyring
chown ceph: /tmp/ceph.mon.keyring

monmaptool --create --add hdmi1 192.168.0.15 --fsid $FS_UUID /tmp/monmap

install -d -o ceph -g ceph /var/lib/ceph/mon/ceph-hdmi1
sudo -u ceph ceph-mon --mkfs -i hdmi1 --monmap /tmp/monmap --keyring /tmp/ceph.mon.keyring
```

The original guide is missing the `chown ceph:` command, which is required, as
Debian runs all services using the account `ceph`.

## Start the monitor

```bash
systemctl enable ceph-mon@hdmi1.service
systemctl start ceph-mon@hdmi1.service

ceph -s
```

It took me hours to get to this point, as the last command timed out and
failed to connect to the monitor. Make sure to use the correct IP addresses
everywhere. If you still experience problems, use `ceph --admin-daemon
/var/run/ceph/ceph-mon.hdmi1.asok mon_status` to directly connect to the
daemon to get its status.

# Setup Ceph OSDs

```bash
for osd in hdmi2 hdmi3
do
  OSD_UUID=$(uuidgen)
  OSD_ID=$(ceph osd create "$OSD_UUID")
  ssh "$osd" "install -d -o ceph -g ceph /var/lib/ceph/osd/ceph-$OSD_ID"
  ssh "$osd" "sudo -u ceph ceph-osd -i $OSD_ID --mkfs --mkkey --osd-uuid $OSD_UUID"
  scp "$osd:/var/lib/ceph/osd/ceph-$OSD_ID/keyring" "osd-$OSD_ID.keyring"
  ceph auth add osd.$OSD_ID osd 'allow *' mon 'allow rwx' -i "osd-$OSD_ID.keyring"
  ceph osd crush add-bucket "$osd" host
  ceph osd crush move "$osd" root=default
  ceph osd crush add "osd.$OSD_ID" 1.0 host="$osd"

  ssh "$osd" "systemctl enable ceph-osd@$OSD_ID.service"
  ssh "$osd" "systemctl start ceph-osd@$OSD_ID.service"
done
```

Again use `ceph -s` to verify the OSDs are setup correctly. In my first try I
forgot to add the `[osd.X] host = hdmiY` entries in `ceph.conf` and the nodes
never left the `booting` state.

# Setup libvirt RBD storage pool

Following [RBD](https://docs.ceph.com/docs/jewel/rbd/libvirt/) create the
pool:

## Create pool

```bash
ceph osd pool create libvirt-pool 128 128
ceph osd lspools
```

## Create credentials

```bash
ceph auth get-or-create client.libvirt mon 'allow r' osd 'allow class-read object_prefix rbd_children, allow rwx pool=libvirt-pool'
ceph auth list
```

## Test create an image

```bash
qemu-img create -f rbd rbd:libvirt-pool/new-libvirt-image 2G
rbd -p libvirt-pool ls
```

## Setup secret for libvirt

```bash
SECRET_UUID=$(uuidgen)
cat >secret.xml <<__XML__
<secret ephemeral='no' private='no'>
  <uuid>$SECRET_UUID</uuid>
  <usage type='ceph'>
    <name>client.libvirt secret</name>
  </usage>
</secret>
__XML__
virsh secret-define --file secret.xml
virsh secret-set-value --secret "$SECRET_UUID" --base64 "$(ceph auth get-key client.libvirt)"
```

## Setup libvirt storage pool

```bash
cat >pool.xml <<__XML__
<pool type="rbd">
  <name>myrbdpool</name>
  <source>
    <name>libvirt-pool</name>
    <host name='192.168.0.15'/>
    <auth username='libvirt' type='ceph'>
      <secret uuid='$SECRET_UUID'/>
    </auth>
  </source>
</pool>
__XML__
virsh pool-define pool.xml
pool-start myrbdpool
virsh vol-list myrbdpool
```

Do not use the fully qualified host name (FQHN) in `host/@name`:
`libvirtd` resolved it to `localhost` and tried to connect the monitor at
`127.0.0.1:6789`, where `ceph-mon` is **not**
[listening](https://docs.ceph.com/docs/jewel/rados/configuration/network-config-ref/#bind):
The monitor is only bound to `192.168.0.15`, but not `127.0.0.1` (or
`0.0.0.0`).

# Using the image

The test image can be used by any VM after adding the volume to the domain
description, which can be edited using `virsh edit $VM`:

```xml
<domain type='kvm' ...>
...
  <devices>
    ...
    <disk type='network' device='disk'>
      <driver name='qemu' type='raw' discard='unmap'/>
      <auth username='libvirt'>
        <secret type='ceph' uuid='$SECRET_UUID'/>
      </auth>
      <source protocol='rbd' name='libvirt-pool/new-libvirt-image'>
        <host name='192.168.0.15'/>
      </source>
      <target dev='sdb' bus='scsi'/>
    </disk>
    ...
    <controller type='scsi' index='0' model='virtio-scsi'/>
    ...
  </devices>
</domain>
```

Notice that the `<auth>...</auth>`-section between the storage **pool** and
storage **volume** are slightly different: For the pool the `type='ceph'`
attribute belongs on the `<auth/>` element, for the volume to the
`<secret/>`element.

I'm also using the `virtio-scsi` controller here as it supports the
`discard='unmap'` option, which is required for `fstrim` to work. If that is
not important for you, you can use other buses like `virtio`, `ide` or `sata`.

{% include abbreviations.md %}
