# Makefile for POS version 0.08 (resurrected from my backups)
# Jason Ernst, 2018
#
# build tested using Ubuntu (64 or 32-bit)
# requires: nasm
# optional for emulation: bochs bochs-sdl
ASM = nasm
IDIR = src/include
CC = gcc
LD = ld

# some of these flags are required so that libraries are not included
# for instance, nostartfiles, nodefaultlibs...but others may be removed
CFLAGS = -I $(IDIR) -m32 -c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin

# finds all of the source files so that we don't need to manually specify when new sources are added
SRC = $(shell find . -name *.c)
CLEANOBJ = $(shell find . -name *.o)
OBJ = $(SRC:%.c=%.o)

# filter out kernel.o so that we can manually put it at front of list for linking
# (otherwise main is not put at correct memory address)
KERNELOBJ = ./src/kernel.o
FILTEROBJ = $(filter-out $(KERNELOBJ), $(OBJ))

# directory name where we will mount a "virtual" floppy for creating OS 
# image
BUILDDIR = build
FLOPPYDIR = floppy

all: prep stage1 stage2 kernel oscopy

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

# this compiles all c source files into object files
%.o: %.c
	@$(CC) -c $< -o $@ $(CFLAGS)

# compile the kernel! :D
kernel: kernel.bin

# kernel(main) is loaded at 0x1400 - note the order of linking here: kernel.o must be first!
# also we need to remove the note and comment section from the elf and switch to a flat binary (not quite sure why atm - investigate in future?)
# util.s contains some assembly for loading the gdt, handing exceptions and irqs that cannot
# be done easily in c, so we link them into the kernel here manually
kernel.bin: ${OBJ} src/asm/interrupt.s
	@echo -n "Compiling kernel..."
	@nasm src/asm/interrupt.s -o $(BUILDDIR)/interrupt.o -f elf32
	@ld -m elf_i386 -o $(BUILDDIR)/KERNEL.BIN -Ttext 0x1400 -e main src/kernel.o $(BUILDDIR)/interrupt.o $(FILTEROBJ)
	@objcopy -R .note -R .comment -S -O binary $(BUILDDIR)/KERNEL.BIN
	@rm $(OBJ)
	@rm -rf $(BUILDDIR)/interrupt.o
	@rm -rf src/asm/interrupt.o
	@echo "done"

# Here we mount the floppy.img file which only has stage1 atm. We can then let Ubuntu treat
# it as a FAT12 filesystem and copy STAGE2.BIN and KERNEL.BIN (and any other files in the future)
# onto the image just like it was a real floppy or hard disk.
oscopy: $(BUILDDIR)/STAGE2.BIN
	@echo -n "Copying OS files to disk image..."
	@sudo mount $(BUILDDIR)/floppy.img $(BUILDDIR)/floppy -o loop
	@sudo cp $(BUILDDIR)/STAGE2.BIN $(BUILDDIR)/floppy
	@sudo cp $(BUILDDIR)/KERNEL.BIN $(BUILDDIR)/floppy
	@sleep 2
	@sudo umount -d $(BUILDDIR)/floppy
	@echo "done."
	@echo "To copy to an actual floppy disk type 'make disk'."

clean:
	@rm -rf $(BUILDDIR)
	@rm -rf $(CLEANOBJ)
