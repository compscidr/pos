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
