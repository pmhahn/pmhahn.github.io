---
title: 'Linux Kernel Module Symbol Versioning'
date: '2025-08-23T12:59:00+02:00'
layout: post
categories: linux
excerpt_separator: <!--more-->
---

The Linux kernel itself and its modules may export symbols, so that other modules can import and use them.
As the functions are written in C, it is important that the function signature matches:
- the number of arguments must match
- the ordering of the arguments must match
- the data types must match, which includes the structure and layout of all input and output parameters

If any of them changes, the _Application Binary Interface_ (ABI) changes and you risk crashing the kernel.
If you're lucky, recompiling the kernel and the modules is enough for both ends to pick up the new _Application Programming Interface_ (API).

To detect such breaking changes, the Linux kernel can be compiled with `CONFIG_MODVERSIONS` enabled:
This calculates a _Cyclic Redundancy Check_ (CRC) checksum over the function signature and embeds this information with the kernel and the modules.
The dynamic linker of the Linux kernel checks, that for each requested symbol its CRC matches the CRC of the Linux kernel or already loaded modules.
A module is only loaded, if a match is found for all symbols.
Otherwise loading fails.

<!--more-->

## Rust goes DWARF

The mechanism described here does not work with Rust.
As such the Linux kernel learned a new trick and can use the DWARF (_Debugging With Arbitrary Record Formats_) debugging information to calculate the CRC.
When `CONFIG_RUST` is enabled, `gendwarfksyms` is used instead of `genksyms`.
Both versions are incompatible as they calculate different CRCs for the same function.
But they work similar enough, so I will not go into details here.
If you're interested, look for `CONFIG_EXTENDED_MODVERSIONS`.

## Executable and Linkable Format

Linux Kernel modules object files using the _Executable and Linkable Format_ (ELF).
Instead of using the well-known suffix `.o`, they use the suffix `.ko`, but are otherwise the same.
They are comprised of multiple _sections_ containing executable code, read-only constants, initialized data and other informations required for linking.

<details><summary>Example: ELF sections of a Linux Kernel Module</summary>
<div markdown="1">

```console
$ objdump --section-headers --wide avm-modver.ko
$ LC_ALL=C readelf --wide --section-headers avm-modver.ko
There are 38 section headers, starting at offset 0x25d80:

Sections Header:
  [Nr] Name                              Type     Addresse    Off   Size ES Flg Lk Inf Al  Usage
🔵[ 0]                                   NULL            0      0      0  0      0   0  0  ELF header
🟠[ 1] .note.gnu.build-id                NOTE            0     40     24  0   A  0   0  4  unique build ID bitstring
🟣[ 2] .note.Linux                       NOTE            0     64     30  0   A  0   0  4  Architecture data
🟢[ 3] .text                             PROGBITS        0     a0     1f  0  AX  0   0 16  Code
⚪[ 4] .rela.text                        RELA            0  14e50     30 18   I 35   3  8
🔴[ 5] __ksymtab                         PROGBITS        0     c0      c  0   A  0   0  4  EXPORT_SYMBOL
⚪[ 6] .rela__ksymtab                    RELA            0  14e80     48 18   I 35   5  8
🔴[ 7] __kcrctab                         PROGBITS        0     cc      4  0   A  0   0  4  CRC
🟣[ 8] __mcount_loc                      PROGBITS        0     d0      8  0   A  0   0  1  ftrace()
⚪[ 9] .rela__mcount_loc                 RELA            0  14ec8     18 18   I 35   8  8
🟣[10] .modinfo                          PROGBITS        0     d8     92  0   A  0   0  1  MODULE_INFO
🟣[11] .return_sites                     PROGBITS        0    16a      4  0   A  0   0  1  Live patching
⚪[12] .rela.return_sites                RELA            0  14ee0     18 18   I 35  11  8
🟣[13] .call_sites                       PROGBITS        0    16e      4  0   A  0   0  1  Live patching
⚪[14] .rela.call_sites                  RELA            0  14ef8     18 18   I 35  13  8
🔴[15] __ksymtab_strings                 PROGBITS        0    172      d  1 AMS  0   0  1  EXPORT_SYMBOL
🔴[16] __versions                        PROGBITS        0    180     51  0   A  0   0 32  CRC
🟣[17] __patchable_function_entries      PROGBITS       58    1d8      8  0 WAL  3   0  8  NOPs
⚪[18] .rela__patchable_function_entries RELA            0  14f10     18 18   I 35  17  8
⚫[19] .data                             PROGBITS        0    1e0      0  0  WA  0   0  1  Initialized data
🟣[20] .gnu.linkonce.this_module         PROGBITS        0    200    500  0  WA  0   0 64
⚫[21] .bss                              NOBITS          0    700      0  0  WA  0   0  1  Uninitialized data
🟠[22] .debug_info                       PROGBITS        0    700   b51f  0      0   0  1
⚪[23] .rela.debug_info                  RELA            0  14f28   fc00 18   I 35  22  8
🟠[24] .debug_abbrev                     PROGBITS        0   bc1f    71e  0      0   0  1
🟠[25] .debug_aranges                    PROGBITS        0   c33d     50  0      0   0  1
⚪[26] .rela.debug_aranges               RELA            0  24b28     48 18   I 35  25  8
🟠[27] .debug_line                       PROGBITS        0   c38d    3be  0      0   0  1
⚪[28] .rela.debug_line                  RELA            0  24b70   1050 18   I 35  27  8
🟠[29] .debug_str                        PROGBITS        0   c74b   7792  1  MS  0   0  1
🟠[30] .debug_line_str                   PROGBITS        0  13edd    943  1  MS  0   0  1
🟡[31] .comment                          PROGBITS        0  14820     58  1  MS  0   0  1  Compiler version
🟡[32] .note.GNU-stack                   PROGBITS        0  14878      0  0      0   0  1  Stack hardening flag
🟠[33] .debug_frame                      PROGBITS        0  14878     40  0      0   0  8
⚪[34] .rela.debug_frame                 RELA            0  25bc0     30 18   I 35  33  8
🔵[35] .symtab                           SYMTAB          0  148b8    438 18     36  40  8  Symbols
🔵[36] .strtab                           STRTAB          0  14cf0    15f  0      0   0  1  Symbol names
🔵[37] .shstrtab                         STRTAB          0  25bf0    18b  0      0   0  1  Section names
Key to Flags:
  Write, Alloc, eXecute, Merge, Strings, Info, Link order, extra Os processing required, Group, TLS,
  Compressed, x=unknown, o=OS specific, Exclude, mbinD, large, processor specific
```

- 🔴 Linux kernel module specific sections
- 🟣 Linux specific sections
- ⚫ data
- 🟢 executable code
- ⚪ relocations
- 🟠 debug information
- 🟡 compiler information
- 🔵 ELF

</div>
</details>

The _section names_ have varying lengths.
As such the names are collected in their own section called `.shstrtab`, which is referenced by index in the ELF file header.
All sections are listed in the _section header table_ and their names are referenced by offset.
Run `readelf -p .shstrtab avm-job.ko` to dump those names.

Similar for symbols:
There names are collected in the section `.strtab` and referenced via offset from `.symtab`.
Run `readelf -p .strtab avm-job.ko` to dump those names.

`.symtab` contains all symbols (and `.strtab`) their names.
When _shared objects_ (`.so`) are used, the linker moves those symbols to `.dynsym` and their names to `.dynstr`.
Already resolved symbols may be removed respectively both tables `.symtab` and `.strtab` may be stripped completely.

The remaining dynamic symbols are only resolved by the _dynamic linker_, when section is loaded.
The _dynamic linker_ has to go through the section and substitute the placeholders with the then correct address.
For that the ELF file contains the _relocation sections_, of which there are two types:
`REL` (relocation without addend) and `RELA` (relocation with addend), which allows to add an additional constant.
Either one may be used per section and each table references a _symbol table_, which gets used.

Not all of them are loaded into memory respectively are freed again, when they are no longer needed by the linker.
Only those sections, which contain information that is necessary for runtime execution of the file, are kept.
Multiple (similar) sections can be combined and are then called _segments_.
But that is only relevant for fully linked executables: Only they have a _program header_

References to functions are then resolved by the linker and the place-holders get replaced by the real addresses.
This is where versioning kicks in.

## Anatomy of a Linux kernel module

When you write and export a function in the Linux kernel or an module, the following happens:
```c
void my_function(void) {
    return;
}
```
1. The compiler/assembler puts the code into the `.text` section.
2. The name of the function is added to the `.strtab` section.
3. An entry is added to the `symtab` section linking the offset within the `.text` section to the name via its offset in the `strtab` section.

Using `EXPORT_SYMBOL` adds more magic:
```c
#include <linux/module.h>
EXPORT_SYMBOL(my_function);
```
1. It puts the name of the function into a section called `__ksymtab_strings`.
    ```console
    $ LC_ALL=C readelf --wide --string-dump=__ksymtab_strings avm-modver.ko
    String dump of section '__ksymtab_strings':
      [     0]  my_function
    ```
2. It creates a new section called `__ksymtab+my_function` with a single `struct kernel_symbol` linking the address of the function to its name.
    Later on these sections will be collected by the linker script `scripts/module-common.lds` and will be put into the section called `__ksymtab`.
    Similar happens for `EXPORT_SYMBOL_GPL` and `EXPORT_SYMBOL_GPL_FUTURE` and `EXPORT_SYMBOL_NS`, but with different prefixes.
    ```console
    $ LC_ALL=C readelf --wide --relocated-dump=__ksymtab --relocs avm-modver.ko | grep -A4 __ksymtab
    Relocation section '.rela__ksymtab' at offset 0x14fc8 contains 3 entries:
        Offset             Info             Type               Symbol's Value  Symbol's Name + Addend
    0000000000000000  0000002e00000002 R_X86_64_PC32          0000000000000010 my_function + 0
    0000000000000004  0000001b00000002 R_X86_64_PC32          0000000000000000 __kstrtab_my_function + 0
    0000000000000008  0000001c00000002 R_X86_64_PC32          000000000000000c __kstrtabns_my_function + 0
    --
    Hex dump of section '__ksymtab':
      0x00000000 10000000 fcffffff 04000000          ............
    ```

Too see more details, use `make avm-modver.i` to run the pre-processor and to get the intermediate file, where all macros have been expanded.

With `CONFIG_MODVERSIONS` enabled even more magic happens.
If a module uses `EXPORT_SYMBOL`, then `genksyms` is called.
The source code of the module is pre-processed again via `cpp`, but with a different definition for `EXPORT_SYMBOLS`.
1. For each function exported via `EXPORT_SYMBOL` a CRC for the function signature is computed by parsing the C function call.
    A new section called `___kcrctab+my_function` with a single `long` containing the CRC is created.
    Later on these sections will be collected by the linker script `scripts/module-common.lds` and will be put into the section called `__kcrctab`.
    Similar happens for `EXPORT_SYMBOL_GPL` and `EXPORT_SYMBOL_GPL_FUTURE` and `EXPORT_SYMBOL_NS`, but with different prefixes.
2. For each used symbol the CRC is looked up in the `Module.symvers` files.
    They are created as part of the kernel or any module compilation process when `CONFIG_MODVERSIONS` is enabled.
    The file collects the CRC and module path for each symbol.
    The symbol name and its CRC is collected in a `const char __versions[]` array in section `__versions`.

## Module loading

When a kernel module is loaded, the Linux kernel linker resolves all dynamic symbols of the module.
It looks up each unresolved symbol from `.symtab` and resolves it to all symbols loaded so far.
You can view them from user-space in `/proc/kallsyms`.

In addition to that simple lookup the loader also checks the modules licence from `.modinfo`:
Symbols exported via `EXPORT_SYMBOL_GPL` can only be resolved if the module has `MODULE_LICENCE("GPL")` and such.

When `CONFIG_MODVERSIONS` is enabled, the linker inside the Linux kernel also checks the CRC:
For every undefined symbol there is a matching entry for it in section `__versions`, which contains the CRC of the symbol from compile time.
```console
$ LC_ALL=C readelf --wide -s avm-modver.ko | grep UND
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND
    42: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND __fentry__
    43: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND _printk
    44: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND __x86_return_thunk
```

But there are two different layouts used:

### Upstream Linux Kernel

```console
$ LC_ALL=C readelf --wide --hex-dump=__versions avm-modver.ko
Hex dump of section '__versions':
  0x00000000 bb6dfbbd 00000000 5f5f6665 6e747279 .m......__fentry
  0x00000010 5f5f0000 00000000 00000000 00000000 __..............
  0x00000020 00000000 00000000 00000000 00000000 ................
  0x00000030 00000000 00000000 00000000 00000000 ................
  0x00000040 d87e9992 00000000 5f707269 6e746b00 .~......_printk.
  0x00000050 00000000 00000000 00000000 00000000 ................
  0x00000060 00000000 00000000 00000000 00000000 ................
  0x00000070 00000000 00000000 00000000 00000000 ................
  0x00000080 cb8119bf 00000000 6d6f6475 6c655f6c ........module_l
  0x00000090 61796f75 74000000 00000000 00000000 ayout...........
  0x000000a0 00000000 00000000 00000000 00000000 ................
  0x000000b0 00000000 00000000 00000000 00000000 ................
```

<!-- ~/REPOS/LINUX/linux/scripts/mod/modpost.c -->
The original Linux kernel uses `const struct modversion_info __version[]`.
The structure has a fixed size of 64 bytes:
- the first 8 bytes contain the CRC.
- the remaining 56 bytes contain the symbol name.

Longer symbol names are not supported and require the use of the _extended modversions_.

### Ubuntu Linux Kernel

```console
$ LC_ALL=C readelf --wide --hex-dump=__versions avm-modver.ko
Hex dump of section '__versions':
  0x00000000 14000000 bb6dfbbd 5f5f6665 6e747279 .....m..__fentry
  0x00000010 5f5f0000 10000000 7e3a2c12 5f707269 __......~:,._pri
  0x00000020 6e746b00 1c000000 ca39825b 5f5f7838 ntk......9.[__x8
  0x00000030 365f7265 7475726e 5f746875 6e6b0000 6_return_thunk..
  0x00000040 18000000 eb7b33e1 6d6f6475 6c655f6c .....{3.module_l
  0x00000050 61796f75 74000000 00000000 00000000 ayout...........
  0x00000060 00
```

<!-- /usr/src/linux-headers-6.8.0-84-generic/scripts/mod/modpost.c -->
Ubuntu has changed this and uses `const char ____versions[]`:
- the first 8 bytes contain the CRC.
- next follows the symbol name with a terminating NUL byte.
- more NUL bytes for padding up to the next address dividable by 4.

Ubuntu changed this to support longer symbol names, which Ubuntu claims is required for RUST support.
See [modpost: support arbitrary symbol length in modversion](https://lists.ubuntu.com/archives/kernel-team/2023-March/137814.html) for details.
This has been reverted by [2039010](https://bugs.launchpad.net/ubuntu/+source/linux/+bug/2039010).

…

## Further reading
- Linux: [Building External Modules - Module Versioning](https://docs.kernel.org/kbuild/modules.html#module-versioning)
- LWN: [A new version of modversions](https://lwn.net/Articles/986892/)
- [Anatomy of the Linux loadable kernel module](https://terenceli.github.io/技术/2018/06/02/linux-loadable-module)
- Linux manual page: [Executable and Linking Format](https://man7.org/linux/man-pages/man5/elf.5.html)
