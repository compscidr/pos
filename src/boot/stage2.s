;-----------------------------------------------------------------------
; Stage 2 - Bootloader - POS, Copyright - Jason Ernst, 2009 - 2018
; 
; Starts in 16-bit mode, loads the kernel (or other program), 
; moves to 32-bit protected mode, executes kernel.
;
; If the org of this code changes, it must match where it is being
; loaded from stage1
;
; The kernel is expected to be in the root directory
; - name specified by [kernelFile], loading address in [kernelPtr]
;
; The boot device # is expected in dl from stage1
;-----------------------------------------------------------------------
[bits 16]
[org 0x0500]

stage2:
  mov si,stageTwoMsg
  call printString

;-----------------------------------------------------------------------
; load-kernel: loads the KERNEL.BIN file off of the FAT root dir into
; kernelPtr
;-----------------------------------------------------------------------
load_kernel:
  mov byte [bootDev],dl       ; save the boot device from stage1
  jmp searchFAT               ; assumes nextStageFile and nextStagePtr

end:
  call killMotor
  pop ax                      ; cleanup stack

;-----------------------------------------------------------------------
; Information collection occurs here
; - cursor position
; - layout of memory
; - video mode changes
;-----------------------------------------------------------------------

;-----------------------------------------------------------------------
; Protected mode initialization
; - enable A20 line
; - load the global descriptor table (GDT)
; - load the interrupt descriptor table (IDT)
; - perform the switch, long jump to pmode to set the code segment
;-----------------------------------------------------------------------
  mov si,pModeMsg
  call printString
  cli
  call emptyKbuffer            ; enable A20
  mov al,0xd1
  out 0x64,al
  call emptyKbuffer
  mov al,0xdf
  out 0x60,al
  call emptyKbuffer
  lgdt [gdtr]                   ; load the GDT
  lidt [idtr]                   ; load the IDT
  mov eax,cr0                   ; switch to protected mode
  or al,1
  mov cr0,eax
  mov ax,[nextStagePtr]         ; pass p-mode the location of kernel
jmp SYS_CODE_SEL:protectedMode

%include 'src/boot/fat12.s'
%include 'src/boot/util.s'

;-----------------------------------------------------------------------
; emptyKbuffer: empties the keyboard buffer (used to enable A20 line)
;-----------------------------------------------------------------------
emptyKbuffer:
	push ax
.emptyAgain:
	dw 0x00eb,0x00eb
	in al,0x64
	test al,2
	jnz .emptyAgain
	pop ax
ret

nextStageFile   db "KERNEL  BIN"
stageTwoMsg		db "Loading KERNEL.BIN...",0
pModeMsg        db "Loaded. Switching to 32-bits",0
bootDev         db 0x00         ; device # of boot device
nextStagePtr    dw 0x1400       ; must match pointer
pointer			dw 0x1400       ; address where kernel will reside
cluster         dw 0x0000       ; cluster of the file to load
SectorsPerTrack dw 18
ReservedSectors dw 1
Sides           dw 2
RootDirEntries  dw 224
buffer          dw 0xC800

;-----------------------------------------------------------------------
; Switch to 32-bit code / data, everything below this point is 32-bits
;-----------------------------------------------------------------------
[bits 32]
protectedMode:
  mov [kernelPtr32],ax
  mov eax,SYS_DATA_SEL
  mov ds,ax
  mov es,ax
  mov fs,ax
  mov gs,ax
  mov ss,ax
	
  mov esp,0x00efffff;
  mov ebp,0x00efffff;
		
  jmp [kernelPtr32]         ; JMP to kernel main
  add esp,4                 ; take care of return value
	
spin: jmp short spin        ; loop in case we get back here

gdtr:
  dw gdtEnd - gdt - 1       ; GDT limit
  dd gdt                    ; GDT base address
  
;-----------------------------------------------------------------------
; Global Descriptor Table (GDT)
;-----------------------------------------------------------------------
gdt:
  times 8 db 0          ; NULL descriptor

SYS_CODE_SEL equ $ - gdt
  dw 0xffff             ; Limit 15:0
  dw 0x0000             ; Base 15:0
  db 0x00               ; Base 23:16
  db 0x9a               ; Type:present, Ring:0, Non-Conforming, Readable
  db 0xcf               ; Page:granular, 32-bit
  db 0x00               ; Base 31:24

SYS_DATA_SEL equ $ - gdt
  dw 0xffff             ; Limit 15:0
  dw 0x0000             ; Base 15:0
  db 0x00               ; Base 23:16
  db 0x92               ; Type:present, Ring:0, Data,Expand-up,Writeable
  db 0xcf               ; Page:granular, 32-bit
  db 0x00               ; Base 31:24
gdtEnd:

idtr:
  dw idtEnd - idt - 1   ; IDT Limit
  dd idt                ; IDT Base Address
  
;-------------------------------------------------------------------------------
; Interrupt Descriptor Table (IDT)
;-------------------------------------------------------------------------------
idt:

idtEnd:

kernelPtr32   dw 0x0000
