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
[org 0x0700]

stage2:
  mov si,stageTwoMsg
  call printString

;-----------------------------------------------------------------------
; load-kernel: loads the KERNEL.BIN file off of the FAT root dir into
; kernelPtr
;-----------------------------------------------------------------------
load_kernel:
  mov byte [bootDev],dl       ; save the boot device from stage1

  jmp searchFAT              ; assumess nextStageFile and nextStagePrt

end:
  jmp $
  
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

%include 'src/boot/fat12.s'
%include 'src/boot/util.s'

nextStageFile   db "KERNEL  BIN"
stageTwoMsg		  db "Stage2 loaded.",13,10,"Loading kernel...",0
diskErrorMsg    db "Disk error loading the kernel, can't continue.'",0
fileErrorMsg    db "KERNEL.BIN missing.",0
rebootMsg       db "Press a key to reboot and try again.",0
bootDev         db 0x00         ; device # of boot device
buffer          dw 0xE000       ; location of the buffer
nextStagePtr    dw 0x1400       ; must match pointer
pointer			    dw 0x1400       ; address where kernel will reside
cluster         dw 0x0000       ; cluster of the file to load
SectorsPerTrack dw 18
ReservedSectors dw 1
Sides           dw 2
RootDirEntries  dw 224
