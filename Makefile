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

all: prep stage1 stage2 oscopy

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

# stage2 looks for the KERNEL.BIN file, switches to pmode and boots into main from kernel.c
stage2: src/boot/stage2.s
	@echo -n "Compiling stage2 bootloader..."
	@rm -rf stage2.bin
	@nasm src/boot/stage2.s -f bin -o $(BUILDDIR)/STAGE2.BIN
	@echo "done".

# Here we mount the floppy.img file which only has stage1 atm. We can then let Ubuntu treat
# it as a FAT12 filesystem and copy STAGE2.BIN and KERNEL.BIN (and any other files in the future)
# onto the image just like it was a real floppy or hard disk.
oscopy: $(BUILDDIR)/STAGE2.BIN
	@echo -n "Copying OS files to disk image..."
	@sudo mount $(BUILDDIR)/floppy.img $(BUILDDIR)/floppy -o loop
	@sudo cp $(BUILDDIR)/STAGE2.BIN $(BUILDDIR)/floppy
#	@sudo cp $(BUILDDIR)/KERNEL.BIN $(BUILDDIR)/floppy
	@sleep 2
	@sudo umount -d $(BUILDDIR)/floppy
	@echo "done."
	@echo "To copy to an actual floppy disk type 'make disk'."

clean:
	@rm -rf $(BUILDDIR)
