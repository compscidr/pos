# Makefile for POS version 0.08 (resurrected from my backups)
# Jason Ernst, 2018
#
# build tested using Ubuntu (64 or 32-bit)
# requires: nasm
# optional for emulation: bochs bochs-sdl
ASM = nasm

# directory name where we will mount a "virtual" floppy for creating OS 
# image
BUILDDIR = build
FLOPPYDIR = floppy

all: prep stage1 

# prepares the directory where we will mount and create our OS image 
# filesystem
prep:
	@test -d $(BUILDDIR) || mkdir $(BUILDDIR)
	@test -d $(BUILDDIR)/$(FLOPPYDIR) || mkdir $(BUILDDIR)/$(FLOPPYDIR)

# stage1 is a FAT12 filesystem head + code that loads the STAGE2.BIN file
# it is zero-padded for the entire 1.4MB floppy and then linux, windows
# or whatever else should be able to mount the image and add files to
# it (ie the stage2.bin file, the kernel, etc.). It then needs 
# to be unmounted and it should be ready to boot.
stage1: src/boot/stage1.s
	@echo -n "Compiling boot sector..."
	@nasm src/boot/stage1.s -f bin -o $(BUILDDIR)/floppy.img
	@echo "done."

clean:
	@rm -rf $(BUILDDIR)
