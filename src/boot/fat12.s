; minimal FAT12 for stage1, stage2 bootloader
; Jason Ernst, Copyright 2018

;-----------------------------------------------------------------------
; Start the process of loading the root directory from disk into memory:
; rootStart = ReservedForBoot + NumberOfFATs * SectorsPerFAT = logic. 19
; rootSectors = RootDirEntries * 32 b per entry / 512 b per sector = 14 
; dataStart = rootStart + rootSectors = logical 19 + 14 = logical 33
;
; assumes that nextStagePtr has the address to load the next stage into
; and assumes that nextStageFile points to the FAT 12 filename of the
; next stage (whether that's STAGE2  BIN or KERNEL  BIN)
;
; Note: this will only work in real mode because it uses the INT13 bios
; handler. We will need to implement a fat12 / floppy driver in protected
; mode, in order to load the kernel to an area beyond 0xFFFF.
;-----------------------------------------------------------------------
searchFAT:

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
  jnc searchRoot            ; search for stage2 on success
  call resetFloppy          ; error: reset floppy and try again
  jnc readRoot
  jmp reboot                ; if we can't reset floppy reboot

searchRoot:
  popa                      ; root dir is now in [buffer]
  mov di,[buffer]           ; so point DI to root dir
  mov cx,word [RootDirEntries]
  mov ax,0                  ; search all 224 entries, 
                            ; starting at offset 0

; assumes si has the next stage filename
nextEntry:
  xchg cx,dx                ; use cx in inner loop
  mov si,nextStageFile	    ; search for next stage filename
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
  mov bx,[nextStagePtr]     ; mem loc to load the next stage to
  mov ah,2
  mov al,1
  push ax                   ; save in case int13 loses it

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

fileErrorMsg    db "Missing.",0
rebootMsg       db "Reboot?",0
diskErrorMsg    db "Disk ERR.",0