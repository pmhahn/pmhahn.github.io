---
title: 'virsh snapshot-edit &#8211;rename considered harmful'
date: '2020-09-03T11:59:58+02:00'
layout: post
categories: virt
---

Recently I stumbled over the possibility to retro-actively modify the snapshot XML data of saved VMs.
This is most helpful for fixing the fallout of [Bug #50412](https://forge.univention.org/bugzilla/show_bug.cgi?id=50414), where several CPUs features related to "TSX" where disabled by the Linux kernel to prevent the [TSX Asynchronous Abort vulnerability](https://wiki.ubuntu.com/SecurityTeam/KnowledgeBase/TAA_MCEPSC_i915).

You can also use the `--rename` option, which allows you to rename your snapshots, but this is **buggy**:
It only renames the XML file and the data within, but **not** the name of the snapshot within the QCOW2 file itself!
Reverting will fail afterwards.

For **inactive** VM you can fix it manually like this:

1. Get the name of your QCOW2 files:
   `virsh domblklist "$VM"`
2. Get the list of snapshots:
   `virsh snapshot-list "$VM"`
3. Compare them to the names stored in the QCOW2 file:
   `qemu-img snapshot -l "$PATH"`
4. For every mismatching snapshot, re-create the snapshot with the new name and delete the old one:
    1. Revert to old snapshot:
       `qemu-img snapshot -a "$OLD" "$PATH"`
    2. Create new snapshot:
       `qemu-img snapshot -c "$NEW" "$PATH"`
    3. Delete old snapshot:
       `qemu-img snapshot -d "$OLD" "$PATH"`

{% include abbreviations.md %}
