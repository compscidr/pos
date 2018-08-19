;-----------------------------------------------------------------------
; Stage 1 16-bit Bootloader - POS, Copyright Jason Ernst, 2009 - 2018
;
; Loads stage2 (or even a kernel) from file in root directory named
; [stageTwoFile] into the address [stageTwoPtr]. The file should be set
; with the same ORG as stageTwoPtr if compiling with nasm or if loading
; a c-program, the code section of the program should be set using a
; ld script when compiling.
;
; Inits all segments to 0x0000 and sets the start of the stack at 0xffff
; The boot drive # is sent to stage2 via dl
;
; Assumes the FAT12 fs conforms to the params specified below, ie) no
; changes in things like BytesPerSector, SectorsPerFAT, etc.
;-----------------------------------------------------------------------
[bits 16]
[org 0x7c00]

jmp short stageOne
nop

;-----------------------------------------------------------------------
; Boot Parameter Block (BPB)
;-----------------------------------------------------------------------
OEMLabel          db "POS BOOT"
BytesPerSector    dw 512          ; size of a single sector (bytes)
SectorsPerCluster db 1
ReservedSectors   dw 1            ; only reserve the boot sector
NumberOfFATs      db 2            ; one redundant FAT for reliability
RootDirEntries    dw 224          ; (224 entries * 32 bytes each)
                                  ; (total 7186 bytes or 14 sectors)
TotalSectors      dw 2880         ; for a standard 1.44mb floppy
MediaDescriptor   db 0xf0			
SectorsPerFAT     dw 9
SectorsPerTrack   dw 18
Sides             dw 2
HiddenSectors     dd 0
LargeSectors      dd 0
DriveNumber       dw 0
Signature         db 0x29         ; indicates following 3 fields
VolumeID          dd 0x31415926   ; are present
VolumeLabel       db "POS v1.0  " ; 11 characters
FileSystem        db "FAT12   "   ; 8 characters
;-----------------------------------------------------------------------
; End of Boot Parameter Block (BPB)
;-----------------------------------------------------------------------

stageOne:
  jmp 0x0:main          ; set the cs to zero
main:                   ; put everything into a known state
  xor ax,ax
  xor bx,bx
  xor cx,cx
  xor dx,dx
  cli                   ; disable interrupts while changing the stack
  mov ds,ax
  mov es,ax
  mov fs,ax
  mov gs,ax
  mov ss,ax
  mov sp,0xffff
  mov bp,0xffff
  sti                   ; re-enable the interrupts

  mov byte [bootDev],dl ; save boot device number
  mov eax,0						  ; required for old BIOS apparently

;-----------------------------------------------------------------------
; Start the process of loading the root directory from disk into memory:
; rootStart = ReservedForBoot + NumberOfFATs * SectorsPerFAT = logic. 19
; rootSectors = RootDirEntries * 32 b per entry / 512 b per sector = 14 
; dataStart = rootStart + rootSectors = logical 19 + 14 = logical 33
;-----------------------------------------------------------------------
init:
  mov si,bootMsg
  call printString

;-----------------------------------------------------------------------
; reboot: waits for user input and reboots the computer
; (using bios int functions)
;-----------------------------------------------------------------------
reboot:
  xor ax,ax
  int 0x16            ; wait for input
  xor ax,ax
  int 0x19            ; reset the system
jmp $                 ; in case something goes wrong hang

;-----------------------------------------------------------------------
; print_string: prints the null-terminated string that SI points to 
; using the int10,0x0e function (teletype)
;-----------------------------------------------------------------------
printString:
  pusha
  mov ah,0x0e
  .printChar:
  lodsb
  cmp al,0
  je .donePrintString
  int 0x10
  jmp short .printChar
  .donePrintString:
  popa
ret
  
;-----------------------------------------------------------------------
; Data
;-----------------------------------------------------------------------
stageTwoFile    db "STAGE2  BIN"
bootMsg         db "Loading stage 2...",0
diskErrorMsg    db "Disk error, press a key to reboot.",13,10,0
fileErrorMsg    db "STAGE2.BIN missing, press a key to reboot.",13,10,0

bootDev         db 0x00     ; device # of boot device
cluster         dw 0x0000   ; cluster of the file to load
pointer         dw 0x0700   ; pointer for placing stage2 in mem
stageTwoPtr     dw 0x0700   ; must match pointer
buffer          dw 0x7e00   ; location of buffer for FAT ops

times 510 - ($-$$) db 0     ; zero pad the rest of the sector
dw 0xaa55                   ; sign. indicates bootable sector
times 512 * 2879 db 0       ; zero pad the rest of floppy
