---
layout: post
title: "Bootable Debian Buster on USB stick"
date: 2019-08-01 14:02:00  +0200
categories: linux debian filesystem
excerpt_separator: <!--more-->
---

Debian Buster was release at the beginning of July 2019.
But how to make a bootable USB stick from it?

<!--more-->

After each Debian release I cannot remember how to update my pocket USB stick,
You can follow <https://www.debian.org/CD/faq/#write-usb> or do the following;:

	cd /media/phahn/INTENSO/

	rm -f mini.iso
	wget http://ftp.debian.org/debian/dists/buster/main/installer-amd64/current/images/netboot/mini.iso
	rm -f vmlinuz
	wget http://ftp.debian.org/debian/dists/buster/main/installer-amd64/current/images/hd-media/vmlinuz
	rm -f initrd.gz
	wget http://ftp.debian.org/debian/dists/buster/main/installer-amd64/current/images/hd-media/initrd.gz

	# rm -f syslinux.cfg
	cat >syslinux.cfg <<__EOF__
	prompt 1
	default vmlinuz
	append initrd=initrd.gz
	__EOF__

	rm -f debian-*-amd64-netinst.iso
	wget https://cdimage.debian.org/debian-cd/current/amd64/iso-cd/debian-10.2.0-amd64-netinst.iso

	rm -f firmware.tar.gz
	wget https://cdimage.debian.org/cdimage/unofficial/non-free/firmware/buster/current/firmware.tar.gz
	rm -rf firmware
	mkdir firmware
	tar xfC firmware.tar.gz firmware/
