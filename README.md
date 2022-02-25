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

| Address                       | Function                              | Size                |
|-------------------------------|---------------------------------------|---------------------|
| 0x0000 - 0x04FF               | BIOS Functions                        | 1280 bytes          |
| 0x0500 - 0x7BFF               | Stage2 Bootloader to be loaded here   | <= 30464 bytes      |
 | 0x7C00 - 0x7DFF               | Stage1 Bootloader loaded here by bios | 512 bytes           |
| 0x7E00 - 0x7E00               | FD Buffer (during stage1)             | 16896 bytes - stack |
| 0x7E00 + stage2 size - 0xFFFF | Stage 1 & Stage 2 Stack               | x bytes             |

In practice, stage2 is only a little bit more than 1 sector, ie) 512 bytes.

Stage 2 and Beyond:

| Address                              | Function                            | Size                        |
|--------------------------------------|-------------------------------------|-----------------------------|
| 0x00000000 - 0x000004FF              | Bios Functions                      | 1280 bytes                  |
| 0x00000500 - 0x00001400              | Stage2 Bootloader                   | 3840 bytes                  |
| 0x00001400 - 0x00007FFF              | Kernel                              | 27647 bytes                 |
| 0x00008000 - 0x0000C800              | FD Buffer (after stage2)            | 18432 Bytes                 |
| 0x0000C800 - 0x0000FFFF - stack size | FD Buffer (during stage2)           | <= 14335 bytes              |
| stack size - 0x0000FFFF              | Stage 2 RT mode stack               | x bytes                     |
| 0x000A0000 - 0x000B0000              | EGA/VGA Memory Graphics Mode        | 65536 bytes                 |
| 0x000B0000 - 0x000B8000              | Mono Text Mode                      | 32768 bytes                 |
| 0x000B8000 - 0x000C0000              | Color Text Mode / CGA Graphics Mode | 32768 bytes                 |
| 0x00100000 - 0x00EFFFFF              | Kernel (TODO)                       | 14680063 bytes ~= 14MB      |
| 0x01000000 - 0x03FFFFFF              | Malloc memory area                  | 50331647 bytes ~= 50MB      |
| 0x04000000 - 0x07FFFFFF              | Stage2 PMode / OS Stack             | 67108863 bytes ~= 67MB      |
| 0x08000000 - 0xFFFFFFFF              | Unused                              | 4160749567 bytes ~= 4160 MB |

Total Required = 128MB

NB: can only move FD buffer to 0x8000 if the second stage <= 512 bytes. Right now its slightly more.

NB: can't put anything above 0xFFFF during stage2 until after pmode switch. This means we're limited to a
kernel which is way less than 65k. Currently its at around 40k, so we'll be in trouble soon. If it gets
much over 40k, the kernel will collide with the FD buffer and everything will break.

One thing we might do is load parts of it, switch to pmode, move it to high mem, switch back to 
real mode (to use int 13 services) and then load more, and repeat.

I tried messing with unreal mode, but that didn't seem to work. Ideally we should probably put the
