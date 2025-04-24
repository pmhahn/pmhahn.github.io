---
layout: post
title: "VirtIO Memory Ballooning"
date: 2019-05-23 15:00:00  +0200
categories: linux virt
excerpt_separator: <!--more-->
---

VirtIO provides *Memory Ballooning*:
the host system can reclaim memory from virtual machines (VM) by telling them to give back part of their memory to the host system.
This is achieved by inflating the *memory balloon* inside the VM, which reduced the memory available to other tasks inside the VM.
Which memory pages are given back is the decision of the guest operating system (OS):
It just tells the host OS which pages it does no longer need and will no longer access.
The host OS then un-maps those pages from the guests and marks them as unavailable for the guest VM.
The host system can then use them for other tasks like starting even more VMs or other processes.

If later on the VM need more free memory itself, the host can later on return pages to the guest and shrink the holes.
This allows to dynamically adjust the memory available to each VM even while the VMs keep running.

<!--more-->

Installation
============
For this mechanism to work the guest OS needs support for with.
The Linux contains support out-of-the-box, for Microsoft Windows the [VirtIO Drivers for Windows](https://www.linux-kvm.org/page/WindowsGuestDrivers/Download_Drivers) contains the `BLNSVR.exe` service.

Configuration
=============
When setting up a VM two memory sizes can be specified (in `libvirt`):

```xml
<memory unit='GiB'>2</memory>
<currentMemory unit='GiB'>1</currentMemory>
```

This configures the maximum size to `2 GiB` and inflates the balloon leave only `1 GiB`.

There are two `libvirt` commands to change those settings:

```bash
virsh setmaxmem --domain $VM --size 3G --config
```

This updates the maximum memory.
Please note that QEMU does **not** allow changing the size while the VM is running, so you need to shutdown the VM first.

But you can modify the balloon by running the following command:

```bash
virsh setmem --domain $VM --size 1500M --current
```

Querying
========
To query the actual memory balloon size, you can use the command `virsh dominfo $VM`:

	Max memory:     2097152 KiB
	Used memory:    2097152 KiBB

There also exists the `virsh dommemstat --domain $VM` command.
The output depends on the fact, if the OS supports the ballooning driver or not.
For example during the boot phase, where GRUB is still running, you will see this:

	actual 67108864
	last_update 0
	rss 67215704

Later on when Linux runs you get this:

	actual 2097152
	swap_in 10936
	swap_out 46632
	major_fault 8471
	minor_fault 111268966
	unused 126932
	available 2052340
	usable 916624
	last_update 1558618059
	rss 1922568

For this to work the ballooning driver must be told to update these statistics on a regular basis.
This can be enabled by running the following command:

```bash
virsh dommemstat --domain $VM --period 5
```

This can be either applied to the running VM using `--live` or to the persistent configuration using `--config`, which updates the [libvirt XML](https://libvirt.org/formatdomain.html#elementsMemBalloon) to look like this:

```xml
<memballoon model='virtio'>
	<stats period='5'/>
</memballoon>
```

Statistics
==========
The following values are always reported:

`actual`:
	The actual memory size in KiB available with ballooning enabled.

`last_update`:
	The time in seconds sine the UNIX epoch (1970-01-01) at which the statistics where last updated.
	`0` means that polling is not enabled.

`rss`:
	The [resident set size](https://en.wikipedia.org/wiki/Resident_set_size) in KiB, which is the number of pages currently "actively" used by the QEMU process on the host system.
	QEMU by default only allocates the pages on demand when they are first accessed.
	A newly started VM actually uses only very few pages, but the number of pages increases with each new memory allocation.

The following values are only reported, if the guest OS supports them and polling is enabled:

`swap_in`, `swap_out`:
	The number of swapped-in and swapped-out pages as reported by the guest OS since the start of the VM.

`major_fault`, `minor_fault`:
	The number of page faults as reported by the guest OS since the start of the VM.
	<br/>
	*Minor* page faults happen quiet often, for example when first accessing newly allocated memory or on copy-on-write.
	They do not required disk IO per-se as only the internal *page table* data structure must be updated.
	<br/>
	*Major* page faults on the other hand require disk IO as some data is accessed, which  must be paged in from disk first.

`unused`:
	Inside the Linux kernel this actually is named `MemFree`.
	That memory is available for immediate use as it is currently neither used by processes or the kernel for caching.
	So it is really *unused* (and is just eating energy and provides no benefit).

`usable`:
	Inside the Linux kernel this is named `MemAvailable`.
	This consists of the *free* space **plus** the space, which can be easily reclaimed.
	This for example includes read caches, which contain data read from IO devices, from which the data can be read again if the need arises in the future.

`available`:
	Inside the Linux kernel this is named `MemTotal`.
	This is the maximum allowed memory, which is slightly less than the currently configured memory size, as the Linux kernel and BIOS need some space for themselves.


Optimizing memory usage
=======================
The memory balloon can be used to shift memory between VMs and the host system.
But finding the right size is tricky and needs some serious thinking.

For further discussion here is an example from my current VM:

	actual    1_536_000  1.5 GB
	available 1_491_188  1.4 GB
	usable      592_880  0.6 GB
	unused      118_808  0.1 GB

* `actual` is the currently configured memory size of 1½ GiB.
* `available` is slightly less as the Linux kernel and BIOS need some extra space ~44 MB.
* `unused` is currently really unused and does not contain any data, not even cached data.
  You can steal this amount of memory by increasing the memory balloon without the guest OS having to do any extra work.
* `usable` is somewhere between `unused` and `available` as it includes data, the guest OS *can* free on demand.
  This includes caches and other data, which can be fetch again from block devices.
  This will then have a higher cost when the data is actually needed again in **the future**.
  On the other hand you might be lucky and the data is not needed again at all.
  But you gain some more free memory to speed up some other tasks **now**.

So this in a bet on the future, which either will pay off or make your VM crawl as slowly as hell.

Shrinking
---------
This is equivalent to increasing the memory balloon by some amount.
That amount can be anything up to `available`, but taking away too much memory from the VM will decrease its performance:
It will start swapping, which requires slow IO operations and will take a lot of more time than accessing memory.

If you become too greedy the Linux kernel might also start killing processes as its out-of-memory-handler will kick in more often.
So probably you should stay below `usable` or even `unused`.

Its probably also a good idea to do it iteratively: Just take some small amount from many VMs and monitor their behavior, if they still run fine with their reduced memory foot-print.
If they still run fine, you can take more memory from them in the next iteration.

Expanding
---------
This is equivalent to decreasing the memory balloon.
You should do that if the VM starts swapping, which indicates that it is running out-of-memory.
So if you see a large increase in `swap_out` you should shrink the memory balloon.
The same applies when `major_fault` increases as major faults need to fetch data from the block device.
In contrast to that `minor_fault` do not access block devices immediately, but still indicate the need for more memory.

Automatic Ballooning
--------------------
Currently (2019-05) that ballooning has to be done manually with QEMU.
There was a [project from 2013](https://www.linux-kvm.org/page/Projects/auto-ballooning) to implement automatic ballooning, but it was never completed.

Details
=======
The naming of the values is somehow confusing, as different components are involved, which name the same thing differently:

* In the Linux kernel the balloon driver is implemented in [linux:drivers/virtio/virtio_balloon.c](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/virtio/virtio_balloon.c#n291).
* It exports some statistics over the VirtIO interface to QEMU, which is implemented in [qemu:hw/virtio/virtio-balloon.c](https://git.qemu.org/?p=qemu.git;a=blob;f=hw/virtio/virtio-balloon.c#l165).
* `libvirt` queries it over the JSON protocol implemented in [libvirt:src/qemu/qemu_driver.c](https://libvirt.org/git/?p=libvirt.git;a=blob;f=src/qemu/qemu_driver.c#l20242).
* The command `virsh dommemstat` is implemented in [libvirt:tools/virsh-domain-monitor.c](https://libvirt.org/git/?p=libvirt.git;a=blob;f=tools/virsh-domain-monitor.c#l356).

The following table shows the values and how they are named in the different components:

| Linux                                 | VirtIO                          | QEMU                    | libvirt                                 | memstat     |
|---------------------------------------|---------------------------------|-------------------------|-----------------------------------------|-------------|
| /proc/vmstat:pswpin                   | `VIRTIO_BALLOON_S_SWAP_IN`      | `stat-swap-in`          | `VIR_DOMAIN_MEMORY_STAT_SWAP_IN`        | swap_in     |
| /proc/vmstat:pswpout                  | `VIRTIO_BALLOON_S_SWAP_OUT`     | `stat-swap-out`         | `VIR_DOMAIN_MEMORY_STAT_SWAP_OUT`       | swap_out    |
| /proc/vmstat:pgmajfault               | `VIRTIO_BALLOON_S_MAJFLT`       | `stat-major-faults`     | `VIR_DOMAIN_MEMORY_STAT_MAJOR_FAULT`    | major_fault |
| /proc/vmstat:pgfault                  | `VIRTIO_BALLOON_S_MINFLT`       | `stat-minor-faults`     | `VIR_DOMAIN_MEMORY_STAT_MINOR_FAULT`    | minor_fault |
| /proc/vmstat:htlb_buddy_alloc_success | `VIRTIO_BALLOON_S_HTLB_PGALLOC` | `stat-htlb-pgalloc`     |                                         |             |
| /proc/vmstat:htlb_buddy_alloc_fail    | `VIRTIO_BALLOON_S_HTLB_PGFAIL`  | `stat-htlb-pgfail`      |                                         |             |
| /proc/meminfo:MemFree                 | `VIRTIO_BALLOON_S_MEMFREE`      | `stat-free-memory`      | `VIR_DOMAIN_MEMORY_STAT_UNUSED`         | unused      |
| /proc/meminfo:MemTotal                | `VIRTIO_BALLOON_S_MEMTOT`       | `stat-total-memory`     | `VIR_DOMAIN_MEMORY_STAT_AVAILABLE`      | available   |
| /proc/meminfo:MemAvailable            | `VIRTIO_BALLOON_S_AVAIL`        | `stat-available-memory` | `VIR_DOMAIN_MEMORY_STAT_USABLE`         | usable      |
| /proc/meminfo:Cached                  | `VIRTIO_BALLOON_S_CACHES`       | `stat-disk-caches`      | `VIR_DOMAIN_MEMORY_STAT_DISK_CACHES`    | disk_caches |
|                                       |                                 |                         | `VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON` | actual      |
| /proc/$pid/status:VmRSS               |                                 |                         | `VIR_DOMAIN_MEMORY_STAT_RSS`            | rss         |
|                                       |                                 | `last-update`           | `VIR_DOMAIN_MEMORY_STAT_LAST_UPDATE`    | last_update |

Linux kernel
------------
The Linux kernel provides the memory statistics itself in [/proc/meminfo](https://www.kernel.org/doc/Documentation/filesystems/proc.txt) in a human readable format:

	MemTotal:        1491188 kB
	MemFree:           83324 kB
	MemAvailable:     435308 kB
	Cached:           575780 kB
	SReclaimable:      53968 kB
	VmallocTotal:   34359738367 kB

More low-level information is available in [/proc/vmstat](https://medium.com/@damianmyerscough/vmstat-explained-83b3e87493b3):

	nr_free_pages 28989
	nr_zone_inactive_anon 97759
	nr_zone_active_anon 91940
	nr_zone_inactive_file 13150
	nr_zone_active_file 95545
	nr_zone_unevictable 0
	nr_zone_write_pending 146
	...
	nr_slab_reclaimable 13522
	nr_slab_unreclaimable 13643
	...
	pswpin 3169
	pswpout 35433
	...
	pgfault 112897306
	pgmajfault 11888
	...
	htlb_buddy_alloc_success 0
	htlb_buddy_alloc_fail 0
	...

{% include abbreviations.md %}
