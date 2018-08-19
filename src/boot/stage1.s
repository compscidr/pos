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

  mov ax,19                 ; see comments above
  call logicalToPhysical
  mov bx,[buffer]           ; set ES:BX to point to buffer
  mov ah,2                  ; int 0x13,2 (read floppy sectors)
  mov al,14                 ; read 14 rootSectors
  pusha                     ; adjust for loop which is
                            ; expecting regs on stack

readRoot:
  popa                      ; in case regs are altered by int13
  pusha
  stc                       ; set CS in case bios doesnt set one
  int 0x13                  ; error (some only clear on success)
  jnc searchRoot           ; search for stage2 on success
  call resetFloppy         ; error: reset floppy and try again
  jnc readRoot
  jmp reboot                ; if we can't reset floppy reboot

searchRoot:
  popa                      ; root dir is now in [buffer]
  mov di,[buffer]           ; so point DI to root dir
  mov cx,word [RootDirEntries]
  mov ax,0                  ; search all 224 entries, 
                            ; starting at offset 0

nextEntry:
  xchg cx,dx                ; use cx in inner loop
  mov si,stageTwoFile	      ; search for stage two filename
  mov cx,11
  rep cmpsb
  je fileFound
  add ax,32                 ; prepare to advance to next entry
  mov di,[buffer]
  add di,ax
  xchg dx,cx                ; get back original cx
  loop nextEntry
  mov si,fileErrorMsg
  call printString
  jmp reboot                ; fail at finding file, reboot

fileFound:
  mov ax,word[es:di + 0x0f] ; fetch cluster & load FAT into RAM
  mov word[cluster],ax
  mov ax,1                  ; sector 1 = 1st sector of 1st FAT
  call logicalToPhysical
  mov bx,[buffer]           ; ES:BX points to buffer
  mov ah,2
  mov al,9                  ; read all 9 sectors of 1st FAT
  pusha

readFAT:
  popa                      ; in case regs are altered by int13
  pusha
  stc
  int 0x13
  jnc preLoad               ; prepare to load file
  call resetFloppy          ; error: reset floppy and try again
  jnc readFAT
  mov si,diskErrorMsg       ; if fail resetting floppy, reboot
  call printString
  jmp reboot
  
preLoad:
  popa
  mov bx,[stageTwoPtr]      ; mem loc to load the next stage to
  mov ah,2
  mov al,1
  push ax                   ; save in int13 loses it

loadFileSector:
  mov ax, word [cluster]    ; convert sector to logical
  add ax,31                 ; 31 = 19 (root dir) + 14 (size)
  call logicalToPhysical
  mov bx, word[pointer]     ; set buffer past what already read
  pop ax
  push ax
  stc
  int 0x13
  jnc computeNextCluster    ; continue on no error
  call resetFloppy          ; reset and try again
  jmp loadFileSector

computeNextCluster:
  mov ax,[cluster]
  xor dx,dx
  mov bx,3
  mul bx
  mov bx,2
  div bx                    ; DX = [cluster] mod 2
  mov si,[buffer]
  add si,ax                 ; AX = word in FAT for 12-bit entry
  mov ax,word[ds:si]
  or dx,dx                  ; if DX = 0 [cluster] is even
  jz even                   ;    DX = 1 then odd

odd:
  shr ax,4                  ; shift away other entry bits
  jmp short nextCluster

even:
  and ax,0x0fff             ; mask out final 4 bits

nextCluster:
  mov word [cluster],ax     ; store cluster
  cmp ax,0x0ff8             ; 0x0ff8 = EOF in FAT12
  jae end
  add word [pointer],512    ; increase buffer pointer 1 sector
  jmp loadFileSector

end:
  call killMotor
  pop ax                    ; clean up stack
  mov dl, byte[bootDev]     ; give stage2 the boot device in dl
  jmp [stageTwoPtr]

;-----------------------------------------------------------------------
; killMotor: attempts to turn off the floppy motor
; does not clobber registers - uses ports
;-----------------------------------------------------------------------
killMotor:
  pusha
  xor al,al
  mov dx,0x3f2
  out dx,al
  popa
ret
  
;-----------------------------------------------------------------------
; reboot: waits for user input and reboots the computer
; (using bios int functions)
;-----------------------------------------------------------------------
reboot:
  mov si,rebootMsg
  call printString
  xor ax,ax
  int 0x16            ; wait for input
  xor ax,ax
  int 0x19            ; reset the system
jmp $                 ; in case something goes wrong hang

;-----------------------------------------------------------------------
; printString: prints the null-terminated string that SI points to 
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
; resetFloppy: attempts to reset the [bootDev] once, flag checks must 
; occur after this function has been called
;-----------------------------------------------------------------------
resetFloppy:
  push ax
  push dx
  xor ax,ax
  mov dl,byte [bootDev]
  stc
  int 0x13
  pop dx
  pop ax
ret

;-----------------------------------------------------------------------
; logicalToPhysical: converts from logical sector into the correct 
; track, head and sector for a floppy disk given the parameters in 
; the BPB
; Input: logical sector in AX
; Output: CH = track / cyl #, CL = sector #, DH = head #, DL = drive #
; Note: still have to set AX after calling this before calling int13
;-----------------------------------------------------------------------
logicalToPhysical:
  push bx
  push ax
  mov bx,ax                   ; save logical sector
  
  ; compute Sector
  xor dx,dx                   ; note: DIV src: AX = DX:AX / src
  div word [SectorsPerTrack]  ; DX = remainder
  add dl,[ReservedSectors]    ; account for this sector
  mov cl,dl                   ; sectors go in cl for int13
  mov ax,bx

  ; compute head / cylinder and track
  xor dx,dx
  div word [SectorsPerTrack]
  xor dx,dx
  div word [Sides]
  mov dh,dl                   ; head / cylinder
  mov ch,al                   ; track

  pop ax
  pop bx
  mov dl, byte [bootDev]      ; set the device to read from
ret
  
;-----------------------------------------------------------------------
; Data
;-----------------------------------------------------------------------
stageTwoFile    db "STAGE2  BIN"
bootMsg         db "Loading stage 2...",0
diskErrorMsg    db "Disk ERR.",0
fileErrorMsg    db "STAGE2.BIN missing.",0
rebootMsg       db "<Enter> to reboot.",0

bootDev         db 0x00     ; device # of boot device
cluster         dw 0x0000   ; cluster of the file to load
pointer         dw 0x0700   ; pointer for placing stage2 in mem
stageTwoPtr     dw 0x0700   ; must match pointer
buffer          dw 0x7e00   ; location of buffer for FAT ops

times 510 - ($-$$) db 0     ; zero pad the rest of the sector
dw 0xaa55                   ; sign. indicates bootable sector
times 512 * 2879 db 0       ; zero pad the rest of floppy
