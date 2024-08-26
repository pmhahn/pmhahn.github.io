---
layout: post
title: "Lenovo ThinkPad L470 Firmware update with Linux"
date: 2018-10-04 09:01:00  +0200
categories: thinkpad UEFI linux firmware update
---

My company notebook (A Lenovo ThinkPad L470) sometimes crashed when I put it into the docking station:
It turn back on, the external monitor turns on, but after that I only see a black screen with the mouse cursor.
Today I had enough and performed the pending firmware update, which also includes the Intel CPU microcode updates.

As a Linux only user performing the firmware update is still fun, as Lenovo provides only a [Windows tool](https://pcsupport.lenovo.com/de/de/downloads/ds120327) or as an alternative a [bootable CD](https://support.lenovo.com/de/de/downloads/ds120328).
There also is the [Linux Vendor Firmware Service](https://fwupd.org/) which nowadays simplified the process and Levono also contributes to it, but not for my model.

```bash
wget https://download.lenovo.com/pccbbs/mobiles/r0guj17wd.iso
```

That gets me the file `r0guj17wd.iso`.

Getting a USB-DVD drive and burning the image to a DVD media I somehow consider wasteful, so I tried to put the image on a spare USB stick (here `/dev/dec`):
```bash
sudo dd bs=1M if=r0guj17wd.iso of=/dev/sdc
```
But the Notebook refused to boot from it.

Next I had a look at it using `isoinfo -d r0guj17wd.iso` which showed the DVD to be empty but only contain an [El Torito](https://de.wikipedia.org/wiki/El_Torito) boot image.
So I extracted the boot image into file:

```bash
geteltorito -o ./leno.boot r0guj17wd.iso
```

Using `file` on it shows it to be a partitioned disk image with only one partition, so I mounted that:

```bash
sudo losetup -P /dev/loop0 ./leno.boot
sudo mount -o ro /dev/loop0p1 /mnt
```

Finally I was able to access the files and copied it onto my Debian 9 Stretch systems EFI partition:

```bash
sudo install -D /mnt/FLASH/NoDCCheck_bootx64.efi /boot/efi/FLASH/NoDCCheck_bootx64.efi
sudo install -D /mnt/FLASH/ShellFlash.efi /boot/efi/FLASH/ShellFlash.efi
sudo install -D /mnt/FLASH/R0GET66W/\$0AR0G00.FL1 /boot/efi/FLASH/R0GET66W/\$0AR0G00.FL1
sudo install -D /mnt/EFI/Boot/Bootx64.efi /boot/efi/EFI/lenovo/bootx64.efi
```

Next I told the firmware to boot into the firmware update tool once:

```bash
sudo efibootmgr --create-only --disk /dev/sda --part 1 --label "FWUpdate" --loader '\EFI\lenovo\bootx64.efi'
sudo efibootmgr --bootnext 0001  # or whatever gets created by the line above
sudo efibootmgr --verbose
```

And finally the reboot:

```bash
sudo reboot
```

The gets you into the firmware update tool:
It only sets up the notebook to perform the firmware update on the **next** reboot, so you have to reboot again once more.

That all went fine so now I'm at version 1.66.

After the update you can remove the files and the boot entry:

```bash
sudo efibootmgr -B -b 0001
sudo rm -rf /boot/efi/FLASH /boot/efi/EFI/lenovo
```

*[CPU]: Central Processing Unit
*[USB]: Universal Serial Bus
*[DVD]: Digital Versatile Disc
