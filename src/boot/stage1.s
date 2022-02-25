;-----------------------------------------------------------------------
; Stage 1 16-bit Bootloader - POS, Copyright Jason Ernst, 2009 - 2018
;
; Loads stage2 (or even a kernel) from file in root directory named
; [stageTwoFile] into the address [nextStagePtr]. The file should be set
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
BytesPerSector    dw 512            ; size of a single sector (bytes)
SectorsPerCluster db 1
ReservedSectors   dw 1              ; only reserve the boot sector
NumberOfFATs      db 2              ; one redundant FAT for reliability
RootDirEntries    dw 224            ; (224 entries * 32 bytes each)
                                    ; (total 7186 bytes or 14 sectors)
TotalSectors      dw 2880           ; for a standard 1.44mb floppy
MediaDescriptor   db 0xf0			
SectorsPerFAT     dw 9
SectorsPerTrack   dw 18
Sides             dw 2
HiddenSectors     dd 0
LargeSectors      dd 0
DriveNumber       dw 0
Signature         db 0x29           ; indicates following 3 fields
VolumeID          dd 0x31415926     ; are present
VolumeLabel       db "POS v1.0   "  ; 11 characters
FileSystem        db "FAT12   "     ; 8 characters
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
  mov eax,0		        ; required for old BIOS apparently

  mov si,bootMsg
  call printString
  
  call searchFAT        ; assumes nextStageFile and nextStagePrt
end:
  call killMotor
  pop ax                ; clean up stack
  
  mov dl, byte[bootDev] ; give stage2 the boot device in dl
  jmp [nextStagePtr]


%include 'src/boot/fat12.s'
%include 'src/boot/util.s'
  
;-----------------------------------------------------------------------
; Data
;-----------------------------------------------------------------------
nextStageFile   db "STAGE2  BIN"
bootMsg         db "Loading STAGE2.BIN...",0

bootDev         db 0x00     ; device # of boot device
cluster         dw 0x0000   ; cluster of the file to load
pointer         dw 0x0500   ; pointer for placing stage2 in mem
nextStagePtr    dw 0x0500   ; must match pointer
buffer          dw 0x7E00   ; location of buffer for FAT ops

times 510 - ($-$$) db 0     ; zero pad the rest of the sector
dw 0xaa55                   ; sign. indicates bootable sector
times 512 * 2879 db 0       ; zero pad the rest of floppy
