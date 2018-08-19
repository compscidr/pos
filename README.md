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
```qemu-system-i386 -drive format=raw,file=build/floppy.img -net nic,model=pcnet -net user```

## Testing in Bochs
```bochs```
When you run bochs, it will start and wait for you to type ```c``` to
continue execution. At this point, it will start loading the 1st stage
bootloader using the legacy bios.

## Memory Map

