# uefi - what needs to be done
- boot device prep:
  - GPT partition scheme
  - FAT partition
  - UEFI application with entry point
    - seems like, at least on NVMEs it should be located at:
      /EFI/BOOT/BOOTX64.efi

Seems like should make a program that prints a character
using VGA and try to load it via UEFI.

An easy first step is to use POSIX-UEFI, GNU-EFI and GCC, but I'd prefer
to build without those if possible, or at least deeply understand how they
work.
