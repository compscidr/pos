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
* bochs bochs-sdl bochsbios vgabios
* qemu-system

## Building 
```make``` will compile the first stage bootload

## Testing in QEMU
Since POS currently only supports rtl8139, it is recommended to use qemu with the network device specified as follows. It is also possible to log the packets to a network dump for debugging after the run.
```qemu-system-i386 -drive file=build/floppy.img,format=raw,if=floppy -net nic,model=rtl8139 -net user,id=u1 -object filter-dump,id=f1,netdev=u1,file=networkdump.dat```

## Testing in Bochs
Currently, bochs only supports the ne2000 network card - this is still a TODO item, so it is not able to use the networking features - but can be used to test non-network stuff.
```bochs```
When you run bochs, it will start and wait for you to type ```c``` to
continue execution. At this point, it will start loading the 1st stage
bootloader using the legacy bios.

## Memory Map
### Stage 1:

| Address                       | Function                              | Size                |
|-------------------------------|---------------------------------------|---------------------|
| 0x0000 - 0x04FF               | BIOS Functions                        | 1280 bytes          |
| 0x0500 - 0x1400               | Stage2 Bootloader to be loaded here   | 3840 bytes          |
| 0x1400 - 0x7BFF               | unused                                | 26623 bytes         |
| 0x7C00 - 0x7DFF               | Stage1 Bootloader loaded here by bios | 512 bytes           |
| 0x7E00 - 0x7E00               | FD Buffer (during stage1)             | 16896 bytes - stack |
| 0x7E00 + stage2 size - 0xFFFF | Stage 1 & Stage 2 Stack               | x bytes             |

Stage two is currently slightly more than 1 sector (its 566 bytes). TODO, should implement an error / warning if it
becomes larger than 3840 bytes.

### Stage 2:

| Address                              | Function              | Size                              |
|--------------------------------------|-----------------------|-----------------------------------|
| 0x00000000 - 0x00000500              | Bios Functions        | 1280 bytes                        |
| 0x00000500 - 0x00001400              | Stage2 ASM Bootloader | 3840 bytes                        |
| 0x00001400 - 0x0000C800              | Stage2.5 C Bootloader | 46080 bytes (currently 31868)     |
| 0x0000C800 - 0x0000FFFF - stack size | FD Buffer             | <= 14335 bytes                    |
| stack size - 0x0000FFFF              | Stage 2 RT mode stack | x bytes (grows from 0x0ffff down) |

Stage 2.5 is currently 31836 bytes. TODO, should implement an error / warning if it becomes larger than 46080 bytes. 

### Stage 2.5:

| Address                              | Function                            | Size                        |
|--------------------------------------|-------------------------------------|-----------------------------|
| 0x00000000 - 0x000A0000              | Low Mem Malloc? (DMA etc)           | 655360 bytes ~= 0.65MB      |
| 0x000A0000 - 0x000B0000              | EGA/VGA Memory Graphics Mode        | 65536 bytes                 |
| 0x000B0000 - 0x000B8000              | Mono Text Mode                      | 32768 bytes                 |
| 0x000B8000 - 0x000C0000              | Color Text Mode / CGA Graphics Mode | 32768 bytes                 |
| 0x00C00000 - 0x00100000              | Unused                              | 11534336 bytes ~= 11.5MB    |
| 0x00100000 - 0x01000000              | Kernel                              | 14680064 bytes ~= 14MB      |
| 0x01000000 - 0x04000000              | Malloc memory area                  | 50331648 bytes ~= 50MB      |
| 0x04000000 - 0x08000000              | Stage2 PMode / OS Stack             | 67108864 bytes ~= 67MB      |
| 0x08000000 - 0xFFFFFFFF              | Unused                              | 4160749568 bytes ~= 4160 MB |

Originally we only had 2 stages, and we loaded the entire kernel under 0xFFFF using the INT13 bios disk functions, 
however, this left only around 40k for the kernel, which eventually ran out. To fix this, we added a 2.5 stage. The
2nd stage only job is to load the stage2.5 into memory at 0x1400, and move into protected mode which sets up the IDT and
GDT. The stage2.5 then implements a fat12 filesystem and loads the kernel into memory at 0x100000.

We now have room for a 14MB kernel, which should be fine for the short term future. The plan is to start moving some of
the utility functions into the file system, and only load them when needed, which should help to keep the kernel size 
down.