FROM ubuntu:focal as base
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    gcc \
    g++ \
    cmake \
    make \
    git \
    ca-certificates \
    parted \
    mtools

# get the POSIX-UEFI helper code
WORKDIR /usr/src
RUN git clone https://git.code.sf.net/p/gnu-efi/code gnu-efi && cd gnu-efi && make && ls -la /usr/src/gnu-efi/x86_64/lib

# build the hello-world app / eventually kernel
WORKDIR /usr/src/pos
COPY . .
RUN gcc -I/usr/src/gnu-efi/inc -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c main.c -o main.o
RUN which ld
RUN ld -shared -Bsymbolic -L/usr/src/gnu-efi/ -T/usr/src/gnu-efi/gnuefi/elf_x86_64_efi.lds crt0-efi-x86_64.o main.o -o main.so -lgnuefi -lefi
RUN objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 main.so main.efi

# build a disk image
RUN dd if=/dev/zero of=/tmp/uefi.img bs=512 count=204800 && \
    parted /tmp/uefi.img -s -a minimal mklabel gpt && \
    parted /tmp/uefi.img -s -a minimal mkpart EFI FAT32 2048s 93716s && \
    parted /tmp/uefi.img -s -a minimal toggle 1 boot && \
    dd if=/dev/zero of=/tmp/part.img bs=512 count=91669 && \
    mformat -i /tmp/part.img -F && \
    mcopy -i /tmp/part.img /usr/src/pos/helloworld.efi :: && \
    dd if=/tmp/part.img of=/tmp/uefi.img bs=512 count=91669 seek=2048 conv=notrunc

# to produce output: DOCKER_BUILDKIT=1 docker build -f Dockerfile --output out .
FROM scratch as export-stage
COPY --from=base /usr/src/pos/helloworld.efi .
COPY --from=base /tmp/uefi.img .