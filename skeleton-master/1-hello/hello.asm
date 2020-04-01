SECTION .data			; Section containing initialised data
msg     db  'Hello world!',0xa                 ;our dear string
len     equ $ - msg                             ;length of our dear string	
SECTION .bss			; Section containing uninitialized data	

SECTION .text			; Section containing code

global 	_start			; Linker needs this to find the entry point!
	
_start:
	nop			; This no-op keeps gdb happy...

        mov     edx,len                             ;message length
    	mov     ecx,msg                             ;message to write
   	mov     ebx,1                               ;file descriptor (stdout)
    	mov     eax,4                               ;system call number (sys_write)
    	int     0x80                                ;call kernel
        
	mov rax, 60		; Code for exit
	mov rdi, 0 		; Return a code of zero
	syscall			; Make kernel call




