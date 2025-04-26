---
layout: post
title: "Secure Boot with Linux"
date: 2017-11-30 13:07:00  +0100
categories: linux security
---

Why
===
Unwanted binaries like viruses should be prevented from loading.
This is known as Secure-Boot.
The (U)EFI firmware only loads binaries signed by the "Platform key" (PK) certificates.
The PK is pre-installed by the manufacturer.
Probably 9x% come with Microsoft Windows pre-installed.
Therefor most PCs come with Microsoft key pre-installed.
For QEMU/KVM there is "OVMF":
It is based on the EDK2 (EFI Development KIT).
It is developed by the "TianoCore" community.
It has not keys pre-installed.

You either have to get your boot-loader (GRUB), Linux-kernel signed by that key owner, or install your own PK.
Microsoft will not sign GRUB2 directly:
Its GPL3 licence could be enforced to reveal your private key, which would break the whole secure boot mechanism.
Therefore exists "SHIM":
It will get loaded by EFI instead of GRUB.
It itself will only than load GRUB and the Linux kernel if they are also signed.
Each Linux distribution builds its own version of SHIM:
It embeds a certificate controlled fully by that distribution.
This certificate is used to sign GRUB and the Linux kernels.
SHIM also allows loading "Machine Owner Keys" (MOK).
This allows using SHIM from one distribution to load kernels from other distributions or your own.

There are two databases within the firmware.
They contain signatures and certificates for EFI drivers and boot-loaders.

* The "Signatures Database" (DB) is a white-list.
  The firmware allows loading of binaries allowed by this database.

* The "Forbidden Signatures Database" (DBX) is a black-list.
  The firmware prevents loading binaries forbidden by this database.

The databases need updates when keys get compromised or new drivers are released.
These updates must be signed cryptographically to be applied by the firmware.
The "Key Exchange Key" (KEK) is used for this.
By default the Microsoft Key is used for this as well.
You can change it if you have control over the PK.

Create PK
---------
We create some keys and certificates for our platform:

```bash
sudo apt install sbsigntool  # Debian Stretch+
mkdir PLATFORM
cd PLATFORM
```

Create new self-signed certificates:

```bash
for n in PK KEK db
do
  openssl genrsa -out "$n.rsa" 2048
  openssl req -new -x509 -sha256 -subj "/CN=$n/" -key "$n.rsa" -out "$n.pem" -days 7300
  openssl x509 -in "$n.pem" -inform PEM -out "$n.der" -outform DER
done
```

Create a GUID to identify us:

```bash
uuidgen >guid
read guid <guid
```

Create empty "EFI signature lists".
This is the format expected by EFI.
It must contain the DER encoded certificate:

```bash
for n in PK KEK db
do
  sbsiglist --owner "$guid" --type x509 --output "$n.esl" "$n.der"
done
```

Create the signed updates for PK, KEK and DB.
Please note, that the PEM encoded certificate must be used:

```bash
for n in PK=PK KEK=PK db=KEK
do
  sbvarsign --key "${n#*=}.rsa" --cert "${n#*=}.pem"  --output "${n%=*}.auth" "${n%=*}" "${n%=*}.esl"
done
```

Install those files into the right location for `sbkesync`:

```bash
install -m 0755 -d etc/secureboot/keys/PK
install -m 0644 -t etc/secureboot/keys/PK PK.auth
install -m 0755 -d etc/secureboot/keys/KEK
install -m 0644 -t etc/secureboot/keys/KEK KEK.auth
install -m 0755 -d etc/secureboot/keys/db
install -m 0644 -t etc/secureboot/keys/db db.auth
install -m 0755 -d etc/secureboot/keys/dbx
sudo mount -t efivarfs none /sys/firmware/efi/efivars
sbkeysync --keystore etc/secureboot/keys --verbose --pk --dry-run
```

After installing the new PK the firmware switches from "setup mode" into "user mode".
You are now no longer able to modify any of variables unless you use one of those signed updates.
Hopefully your firmware provides a mechanism to reset it back into "setup mode".

Sign boot-loader
----------------
GRUB is modular by design.
You must built a customized image:
It must not contain some modules.
They would allow loading an unsigned Linux kernel.
(GRUB has its own mechanism to only load signed things: [Using digital signatures](https://www.gnu.org/software/grub/manual/grub/html_node/Using-digital-signatures.html#Using-digital-signatures), [secure boot with grub and signed Linux and initrd](https://ruderich.org/simon/notes/secure-boot-with-grub-and-signed-linux-and-initrd))

```bash
sudo apt install grub-efi-amd64-bin
cat >grub.cfg <<__CFG__
set debug=linuxefi
echo Loading step 1a
sleep 1
source /efi/univention/grub.cfg
echo Loading step 1b
sleep 1
search --file --set=root /efi/univention/grub.cfg
source /efi/univention/grub.cfg
__CFG__
: >modules
echo acpi efi{fwsetup,_gop,net,_uga} >>modules
echo all_video bitmap{,_scale} font gfx{menu,term{,_background}} jpeg png video{,_bochs,_cirrus,_colors,_fb} >>modules
echo btrfs ext{2,cmd} fat hfsplus iso9660 lvm mdraid{09,1x} part_{apple,gpt,msdos} >>modules
echo bufio disk{,filter} gzio lvm lzopio mdraid{09,1x} memdisk part_{apple,gpt,msdos} search{,_fs_{file,uuid},_label} >>modules
echo crypto gcry_sha512 >>modules
echo chain configfile datetime echo gettext halt keystatus ls{efi{,mmap,systab},sal} loadenv minicmd mmap net normal {password_,}pbkdf2 priority_queue reboot relocator sleep terminal test trig true >>modules
echo linuxefi >>modules
grub-mkimage --format x86_64-efi --output grubx64.efi --config grub.cfg --directory /usr/lib/grub/x86_64-efi --prefix /efi/boot $(<modules)
```

You must now sign your boot-loader to get it loaded:

```bash
sbsign --key db.rsa --cert db.pem --output grubx64.efi.signed grubx64.efi
install -m 644 grubx64.efi.signed /boot/efi/EFI/boot/BOOTX64.EFI
install -m 644 grubx64.efi.signed /boot/efi/EFI/univention/BOOTX64.EFI
```

Microsoft will not do that!
See #Shim below.

`/boot`:
	The traditional directory for Linux kernel.
	May be an extra filesystem readable by GRUB.

`/boot/efi`:
	The "EFI system partition".
	It's a FAT32 file system.

`/boot/efi/EFI`:
	The directory for different boot entries.

`/boot/efi/EFI/BOOT`:
	The default directory for removable devices.

`/boot/efi/EFI/BOOT/BOOTX64.EFI`:
	The default boot entry.
	This is used as the last fall back.

Shim
----
We create our own certificate:

```bash
openssl genrsa -out "shim.rsa" 2048
openssl req -new -x509 -sha256 -subj "/CN=shim/" -key "shim.rsa" -out "shim.pem" -days 7300
openssl x509 -in "shim.pem" -inform PEM -out "shim.der" -outform DER

ln -s ../CA/archive-subkey-public.der shim.der
ln -s ../CA/archive-subkey-public.pem shim.pem
ln -s ../CA/private/archive-subkey-private.key shim.rsa
```

We build our own shim:

```bash
make EFI_PATH=/usr/lib VENDOR_CERT_FILE="shim.der" ENABLE_SBSIGN=1 install-as-data
```

Microsoft signs our build:

```bash
sbsign --key db.rsa --cert db.pem --output shimx64.efi.signed /usr/lib/shim/shimx64.efi
```

Thus it gets loaded by PCs using the Microsoft key.
GRUB needs to be re-signed by our embedded key:

```bash
sbsign --key shim.rsa --cert shim.pem --output grubx64.efi.signed grubx64.efi
install -m 0644 grubx64.efi.signed /boot/efi/EFI/BOOT/grubx64.efi
install -m 0644 grubx64.efi.signed /boot/efi/EFI/univention/grubx64.efi
```

Install shim:

```bash
install -m 0644 shimx64.efi.signed /boot/efi/EFI/univention/shimx64.efi
install -m 0644 shimx64.efi.signed /boot/efi/EFI/BOOT/BOOTX64.efi
```

"Machine Owner Keys" (MOK) allow you to load things yourself.
You can while-list individual binaries by adding their hash.
You can load certificates to allow loading everything signed by them.
This requires the MOK-Manager:

```bash
install -m 0644 /usr/lib/shim/mmx64.efi.signed /boot/efi/EFI/univention/mmx64.efi
install -m 0644 /usr/lib/shim/mmx64.efi.signed /boot/efi/EFI/BOOT/mmx64.efi
```

EFI allows booting different binaries.
Their paths need to be registered as EFI variables.
If they are not configured correctly, nothing gets booted.
Install the "Fallback Manager" which can fix this:

```bash
echo "shimx64.efi,univention,,This is the boot entry for UCS" | iconv -t UCS-2LE >BOOTX64.CSV
install -m 0644 BOOTX64.CSV /boot/efi/EFI/univention/BOOTX64.CSV
install -m 0644 /usr/lib/shim/fbx64.efi.signed /boot/efi/EFI/BOOT/fbx64.efi
```

-----

Sign Linux kernel with key embedded in SHIM:

```bash
sbsign --key shim.rsa --cert shim.pem --detached --output vmlinuz-4.9.0-ucs105-amd64.sig vmlinuz-4.9.0-ucs105-amd64
cp vmlinuz-4.9.0-ucs105-amd64 vmlinuz-4.9.0-ucs105-amd64.efi.signed
sbattach --attach vmlinuz-4.9.0-ucs105-amd64.sig vmlinuz-4.9.0-ucs105-amd64.efi.signed
```

NvVars
------
UEFI requires a writable flash device.
`qemu -bios` will only provide one big read-only ROM.
In that case "OVMF" stores the EFI variables in `/boot/EFI/NvVars`.
The correct way for QEMU is
```
-drive file=/usr/share/OVMF/OVMF_CODE.fd,if=pflash,format=raw,unit=0,readonly=on
-drive file=/home/phahn/.config/libvirt/qemu/nvram/uefi_VARS.fd,if=pflash,format=raw,unit=1
```
This stores the EFI-variables in the `VARS` file.
But this does not implement the restriction, that some variables must only be modified by the boot-service, e.g. all Secure-Boot related ones.
This requires System Management Mode (SMM) to work.
In that case the EFI-variables flash is write protected, so only SMM can write it.
This needs to be turned on:
```
-global driver=cfi.pflash01,property=secure,value=on
```
If OVMF does not support SMM mode, writing to the flash will now fail.
In that case OVMF will fall back to using `NvVars` again.
But the EFI file system is too late for storing the SecureBoot related keys:
After each reboot they will be gone again.
So make sure to use the right version of OVMF with the right flash setting!

{% include abbreviations.md %}
