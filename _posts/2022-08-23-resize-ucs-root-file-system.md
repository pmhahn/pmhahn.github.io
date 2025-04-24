---
title: 'Resize UCS root file system'
date: '2022-08-23T18:13:41+02:00'
layout: post
categories: UCS filesystem
---

Q: How do I resize the root file system of my UCS VM?

A: It depends on if you are using *Logical Volume Manager* (LVM) or not.

## With LVM

By default all of our `ucs-kt-get` templates use LVM.

1. Create a new QCOW2 file:
   `qemu-img create -f qcow2 "/var/lib/libvirt/images/${DOMAIN}-2.qcow2" 100G`.
2. Make sure the file is writable by group `Tech`:
   `chmod 664 "/var/lib/libvirt/images/${DOMAIN}-2.qcow2"`.
3. (Live-)attach the additional volume to the VM:
   `virsh attach-disk --persistent --live --domain "$DOMAIN" --type disk --driver qemu --subdriver qcow2 --cache unsafe --sourcetype file --source "/var/lib/libvirt/images/${DOMAIN}-2.qcow2" --targetbus virtio --target vdb`.
4. Login to the VM as user `root`.
5. Create a *Physical Volume* (PV) signature on the block device — no MBR/GPT partition table needed:
   `pvcreate /dev/vdb`.
6. Add the PV to the existing *Volume Group* (VG):
   `vgextend vg_ucs /dev/vdb`.
7. Resize the existing *Logical Volume* (LV) containing the root file system:
   `lvresize /dev/vg_ucs/root -L +100%FREE`.
8. Resize the file system itself:
   `resize2fs /dev/vg_ucs/root`.

If you get any error message like `Command failed with status code 5` from `vgextend` this might be cause by your file system being too full or mounted read only:
LVM tries to create a backup of the LVM metadata before modifying it.
Try to free some space or try to remount the file system read-write with `mount -o remount,rw /`.

## Without LVM

This only works if the VM does not have snapshots;
they must be removed one-by-one with `virsh snapshot-delete "$DOMAIN" "$SNAPSHOT"` first.

The main problem here is that by growing the underlying QCOW2 image file new free space appears at the end of the disk.
By default the SWAP partition is located there, which must be removed to then grow the root file system, which is located at the beginning of the disk.

1. Shutdown the VM.
2. Find the QCOW2 file of the VM:
   `virsh domblklist "$DOMAIN"`.
3. Get the current allocation size the of QCOW2 image:
   `virsh vol-info "/var/lib/libvirt/images/${DOMAIN}-0.qcow2"`.
4. Grow the image size:
   `virsh vol-resize "/var/lib/libvirt/images/${DOMAIN}-0.qcow2" 100G`.
5. Start the VM.
6. Login to the VM as user `root`.
7. Remove SWAP from `/etc/fstab`.
8. Go into rescue mode:
   `systemctl rescue`.
9. Turn of SWAP:
   `swapoff -a`.
10. Edit the partition table:
    `fdisk /dev/vda`.
    1. Dump the current partition table:
       `p`.
    2. Write down the new size of the disk in sectors and the old start sector of `/dev/vda1`;
       usually `2048`.
    3. Delete the local partition `d 5` containing the SWAP space.
    4. Delete the extended partition `d 2` which contained the logical partition.
    5. Delete the primary partitions `d 1` which contains the root file system.
    6. Re-create the primary partition `n p 1 2048 default No.`.
       This will warn you that the old `ext4` signature was re-detected;
       do **not** delete it.
    7. Write the new partition table `w` and exit `fdisk`.
11. Resize the file system itself:
    `resize2fs /dev/vda1`.
12. `reboot`

The procedure above deletes the SWAP space.
If you need one do not allocate all space when re-sizing partition 1.
Afterwards create another new partition (can also be a primary, no need to create an extended and logical partition again).
Make sure to use partition type `82`.
Run `mkswap /dev/vda2` to initialize it.
Afterwards update `/etc/fstab` to use `/dev/vda2` or lookup the UUID from `/dev/disk/by-uuid/`.

{% include abbreviations.md %}
*[GPT]: Global Partitioning Table
