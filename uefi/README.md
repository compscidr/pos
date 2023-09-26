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

## building the disk image:
https://wiki.osdev.org/UEFI#Platform_initialization

```
DOCKER_BUILDKIT=1 docker build -f Dockerfile --output out .
```

## running the disk image
`apt-get install qemu ovmf`

```
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
cargo install uefi-run
uefi-run -b /usr/share/OVMF/OVMF_CODE.fd -q /usr/bin/qemu-system-x86_64 out/helloworld.efi
```

## running on real hardware
so far no luck :(