---
title: 'Too much UMA frame buffer'
date: '2024-04-11T14:34:39+02:00'
layout: post
categories: linux
---

Some time ago I received a new Lenovo [P14s](https://www.lenovo.com/de/de/p/laptops/thinkpad/thinkpadp/thinkpad-p14s-gen-4-(14-inch-amd)-mobile-workstation/len101t0070) notebook.
My previous [L470](https://www.lenovo.com/de/de/p/laptops/thinkpad/thinkpadl/thinkpad-l470/22tp2tbl470) worked very well for a long time, so I only switch to the new model beginning this year.
Since then I experienced regular stalls:
Mostly during our daily video conferences the laptop locked up for some minutes;
only after 1-2 minutes I was able to resume my work, but found some process(es) gone.

Finally I was able to nail the issue down to a memory issue:
The [Linux kernel out-of-memory-killer](https://www.kernel.org/doc/gorman/html/understand/understand016.html) (OOM-kill) found all memory used, tried to swap, failed, and then killed some process — mostly `thunderbird`.
During that time the system was mostly unresponsive:
the mouse cursor only moved very slowly, but clicking took "infinite" time.

Running `free -h` and `head -n1 /proc/meminfo` only showed the system using **11 GiB**, while the system was supposed to have **16 GiB**.
1 GiB off is expected as that is used for legacy services and video memory, but 5 GiB?
My old L470 had 16 GiB physical memory and reported 15 GiB as usable, which I was expecting also for the new model.

The culprit is the *integrated AMDGPU*, for which 4 GiB are reserved as *UMA frame buffer*.
There’s a setting in the [UEFI firmware](https://download.lenovo.com/bsco/index.html#/graphicalsimulator/ThinkPad%20P14s%20Gen%202%20AMD%20(20A0,20A1)) under *Config → Display*, where you can change the setting.
[AMD](https://www.amd.com/en/support/kb/faq/pa-280 "Advanced Mirco Devices") provides some background, but does not provide any hint on how much memory should be reserved for which use-case.
The only recommendation is *screen-width × screen-height × bytes-per-pixel × factor*, but even for 4K with double-buffering that is only 128 MiB.
4 GiB looks way too much.

[![Lenovo P14s UEFI configuration]({{site.url}}/images/p14suefi-300x193.png)]({{site.url}}/images/p14suefi.png)

As I neither run games on the notebook nor do any graphic heavy work nor use the GPU for ML, I changed the value from *Auto* (obviously *4 GiB*) to *1 GiB* and now have 3 GiB more memory available for Linux to use for running processes.

I also added more swap space, so that Linux has the option to use the NVMe to swap out some unused memory instead of being forced to kill some processes, if memory pressure gets too high.

{% include abbreviations.md %}
