---
title: 'UEFI Secure Boot SBAT'
date: '2021-10-19T11:43:02+02:00'
layout: post
categories: debian
---

Several flaws were found in GRUB, the [GRand Unified Boot-Loader](https://www.gnu.org/software/grub/) in 2020 and 2021.
These can be used for by-pass [Secure Boot]({% post_url 2018-04-11-debian-secure-boot-sprint-2018 %}), which provides a secure path from booting the PC to running Linux:
No unauthorized software like a virus should have a chance to get loaded **before** Linux has been loaded and started.

## Status quo

This is implemented using cryptography:
The UEFI firmware has some built-in certificates – usually one from the hardware manufacturer and one from Microsoft, but if you’re big enough, you can get the manufacturers to build a customized series with your own certificate.
If _Secure Boot_ (SB) is enabled, the firmware will only load software signed by those certificates.
To prevent the malicious software from just turning SecureBoot off, toggling the state is restricted to be physically present:
There is no network accessible API for this, but you must enter the firmware setup by pressing some physical keys.
For a single PC this is okay, but think of an cloud provider having to do this for 1000 servers in person.

As GRUB and the Linux kernel are updated regularly, this would require getting them signed by Microsoft each time.
This would not only delay each release, but also would lead to problems, when a compromised component needs to be revoked:
The hash of each revoked component needs to be added to the _database of revoked signatures_ (DBX), which is stored inside a protected area of the firmware.
The space allocated to that database is very constrained and is already ⅓ full.
Bugs in the UEFI firmware for handling this database have already caused PCs to become un-bootable.

Instead of signing GRUB and/or the Linux kernel directly, a thin software layer called [SHIM](https://github.com/rhboot/shim/) has been added between the UEFI firmware and GRUB:
Each vendor like RedHat, SUSE, Debian, Ubuntu, Univention builds their own SHIM with their own certificate embedded, which is then used to sign all following components like GRUB and the Linux kernel.
Microsoft then signs these SHIMs, so they can get loaded out-of-the-box on most hardware.

This SHIM provides an API to GRUB to load and verify further components like the Linux kernel.
This was done as GRUB is a very complex piece of software and Microsoft requires to code to be reviewed.
By moving the code out of GRUB into a separate layer, this task is a look of easier, as only SHIM must be audited.
GRUB just tells SHIM to load the Linux kernel, verify it and to execute it, if the certificate burned into SHIM matches the certificate, which was used to sign the Linux kernel.

SHIM also provides a mechanism to add additional custom certificates _Machine Owner Key_ (MOK):
This can be used by power-users to build their own kernels, which are then signed with their own custom certificate.
But loading this certificate into SHIM again required physical presence to prevent malicious software from just doing that.

This was deemed enough until the above mentioned security flaws in GRUB were detected.

## The problem

`shim` solves the problem of **adding** additional components to the secure boot chain and removes the need to go to Microsoft and requesting them to sign additional components.
If any of these components can be used to break the secured chain and to modify the environment, before the operation system is started, is must be revoked!
Not only for the future, but also for the past: Otherwise you can still use an old UCS installation media to break any existing setup.

- Any previous Linux kernel, which is vulnerable, needs to be added to the DBX. Until UCS-4 we have 59 versions of `univention-kernel-image-signed` so far.
- Any previous GRUB, which is vulnerable, needs to be added to a DBX. Until UCS-4 we have 21 versions of `grub-efi-amd64-signed` so far.

But there is only **one** bridge between those components and the Microsoft root certificate: Our `shim`.
So Microsoft just revoked our (and RedHats, SUSEs, Debians, Ubuntus, …) `shim` by adding them to the [UEFI Revocation List file](https://uefi.org/revocationlistfile), which is distributed by Windows updates and/or firmware updates (already or in the near future).
Actually 3 certificates and 150 images were revoked, so ⅓ of the space allocated for DBX is now already used.

## The solutions

We must now build a new version of `shim` and get it signed by Microsoft again.
For this we must not simply re-use our old certificate, because that would allow our new `shim` to load the old and broken components, which would get it revoked again instantly.
There are two options:

1. The new `shim` has a built-in deny-list, where we can (and must!) enumerate all known broken binaries published so far.
2. Buy a new certificate and use that one for all future components to sign.

Unknowingly we have already renewed our [Extended Validation Certificate](https://en.wikipedia.org/wiki/Extended_Validation_Certificate) in the past, but never used it.
(Actually I discovered this by accident when we first installed the renewed certificate.
Afterwards we were no longer able to sign components, because our old `shim` no longer matched with the new certificate.
So far we were still signing with the original certificate, which expired 2017.)

### Current time

To prevent Microsoft from having to revoke many SHIMs again [Secure Boot Advanced Targeting](https://github.com/rhboot/shim/blob/main/SBAT.md) (SBAT) has been invented to mitigate this problem.
Each component (`shim`, `grub`, `linux`, …) gets a product specific version number and another version number per vendor (the later is only there if we screw up our patched version, but not upstreams original version).
Each time a (major) new vulnerability is found one of these generation numbers is incremented.
This new minimum requirement is then distributed similar to the DBX as a single EFI variable `/sys/firmware/efi/efivars/SbatLevelRT-605dab50-e046-4300-abb6-3dd810dd8b23`.
The variable is secured by the firmware and can only be updated by signed updates.
Its content is just a list of comma separated entries, where each entry names the minimum required generation for each component.
`shim` compares this minimum generation number before loading an component and only allows this, if the loaded generation is larger or equal to the minimum required generation number.

For example Microsoft thus will be able do revoke all past version of GRUB by just incrementing the minimum required version number for `grub`.
Compared to revoking multiple versions of `shim`, which required adding multiple entries to DBX, this just adds one more entry or even replaces the previous entry, which is much more space efficient.

### The future

The current scheme still requires each vendor to build its own version of `shim` with its own certificate embedded, which then must be reviews my Microsoft before it gets signed.
Currently there are plans to separate the certificate from `shim`:
This would allow all vendors to share one version of `shim` and have Microsoft only sign vendor certificate with their key.
As the certificate is only data compared to code, this would greatly improve and speedup the process.

## Further reading

- [Debian](https://www.debian.org/security/2021-GRUB-UEFI-SecureBoot/)
- [Ubuntu](https://wiki.ubuntu.com/SecurityTeam/KnowledgeBase/GRUB2SecureBootBypass2021)
- [SUSE](https://www.suse.com/support/kb/doc/?id=000019892)
- [RedHat](https://access.redhat.com/security/vulnerabilities/RHSB-2021-003)
- [Eclypsium](https://eclypsium.com/2021/04/14/boothole-how-it-started-how-its-going/)
