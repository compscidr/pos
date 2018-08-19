# POS: Parallel Operating System
This OS was intended to be a baremetal OS designed to be used for 
multi-system processing. It was motivated by all of the bloat and extra
configuration that is needed with a typical linux system to get this
kind of thing to work

It was originally started during my PhD, way back in 2009 where I worked
on it on and off for a couple of years before I got too busy with a 
robotics company and a mesh networking company to continue it, but I am
resurrecting it now as a hobby.

For easy testing, it is setup to work inside of bochs, qemu or
virtualbox so that it can be run locally on the machine without having
to reboot out of the build enivronment, however it is also designed to
be run on real machines which are not baremetal.

The build toolchain has been tested with Ubuntu 18.04 and requires
* nasm
* build-essential

The OS can be tested using either qemu or bochs which require the
following packages:
* bochs bochs-sdl
* qemu qemu-system-i386

## Building 
```make``` will compile the first stage bootload

## Testing in QEMU
Since POS currently only supports rtl8139, it is recommended to use qemu with the network device specified as follows. It is also possible to log the packets to a network dump for debugging after the run.
```qemu-system-i386 -fda build/floppy.img -net nic,model=rtl8139 -net user,id=u1 -object filter-dump,id=f1,netdev=u1,file=networkdump.dat```

## Testing in Bochs
Currently, bochs only supports the ne2000 network card - this is still a TODO item, so it is not able to use the networking features - but can be used to test non-network stuff.
```bochs```
When you run bochs, it will start and wait for you to type ```c``` to
continue execution. At this point, it will start loading the 1st stage
bootloader using the legacy bios.

## Memory Map
Stage 1:
| Address | Function  | Size  |
|---|---|---|
|0x0000 - 0x04FF | Reserved | 1280 bytes |
|0x0500 - 0x10FF | Stage2 Bootloader to be loaded here | 3072 bytes |
|0x1100 - 0x16FF | Unused | 1536 bytes |
|0x1700 - 0x18FF | Kernel Params | 512 bytes |
|0x1900 - 0x7BFF | Unused | 25344 bytes |
|0x7C00 - 0x7DFF | Stage1 Bootloader loaded here by Bios | 512 bytes |
|0x7E00 - 0xBFFF | Disk buffer for loading Stage2 | 16896 bytes |
|0xC000 - 0xFFFF | OS Stack | 16384 bytes |

Stage 2 and Beyond:
| Address | Function  | Size  |
|---|---|---|
|0x0000 - 0x04FF | Reserved | 1280 bytes |
|0x0500 - 0x10FF | Stage2 Bootloader (unused by kernel) | 3072 bytes |
|0x1100 - 0x16FF | Unused | 1536 bytes |
|0x1700 - 0x18FF | Kernel Params | 512 bytes |
|0x1900 - 0xBFFF | Kernel | 42751 bytes |
|0xC000 - 0x0FFFFF | Unused | 999423 bytes |
|0x00100000 - 0x03FFFFF | Malloc memory area | 3145728 bytes ~= 3MB |
|0x00400000 - 0x00EFFFFF | OS Stack | 11534336 bytes ~= 10MB |
|0x00F00000 - 0xFFFFFFFF | Unused | 4279238655 bytes ~= 4280 MB |
