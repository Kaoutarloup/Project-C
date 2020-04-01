SECTION .data ; Section containing initialised data

SECTION .bss ; Section containing uninitialized data
	MAXARGS equ 10 ; Number of arguments we support
	ArgCount: resq 1 ; Number of arguments passed to program
	ArgPtrs: resq MAXARGS ; Table of pointers to arguments
	ArgLens: resq MAXARGS ; Table of argument lengths

	RESULTLEN equ 10 	; result string length
	Result: resb RESULTLEN ; reserve result string
  
	DATABUFFERLEN equ 100000001 	; data buffer length
	DataBuffer: resb DATABUFFERLEN ; reserve buffer

SECTION .text ; Section containing code
	global _start ; Required so linker can find entry point

_start:
	nop ; This no-op keeps gdb happy...

; argument count to stack
	pop RCX ; TOS contains the argument count
	mov qword [ArgCount],RCX ; Save argument count in variable
	
; Pop arguments with loop to ArgPtrs
	xor RDX,RDX ; Zero a loop counter

SaveArguments:
	pop qword [ArgPtrs + RDX*8]  ; Pop an argument into the memory table
	inc RDX ; Increment the counter to the next argument
	cmp EDX,ECX ; If the counter != the argumemt count, loop back
	jb SaveArguments

; Calculate argument length
	xor RAX,RAX ; Clear AL to 0
	xor RBX,RBX ; Pointer table offset starts at 0
	
Scan:
	mov RCX,0000ffffh ; Limit search to 65535 bytes
	mov RDI,qword [ArgPtrs+RBX*8] ; Put string address to RDI
	mov RDX,RDI ; Copy address to RDX
	cld ; Set search direction to upmemory
	repne scasb ; Search for null in RDI

	mov byte [RDI-1],10 ; Store an EOL where the null used to
	sub RDI,RDX ; Subtract portion of 0 from start address
	mov qword [ArgLens+RBX*8],RDI ; Put length of arg
	inc RBX ; Increment argument counter
	cmp RBX,[ArgCount] ; If not arg counter exceeds argument, loop back
	jb Scan
	
; arguments are now in ArgPtrs, ArgLens
	mov RAX,[ArgCount]
	mov RBP,[ArgPtrs+8]	; pointer of arg[1]
	mov RSI,[ArgLens+8] ; input string index = string length (ArgLens[1]), so inc by 8

;; String to int
ToIntRemoveLastChar:	; remove end-of-line character
	sub RSI,1		; last char
	cmp byte[RBP+RSI],30H 	; compare with  '0'
	jb ToIntRemoveLastChar	; jump if below
	cmp byte[RBP+RSI],39H 	; compare with  '9'
	ja ToIntRemoveLastChar	; jump if above
	
	xor EAX,EAX		; clear high byte
	xor EBX,EBX		; clear result register
	mov ECX,1		; weight is 1
	
ToIntNext:
	xor EAX,EAX	  ; clear higher bytes for multiplication with EAX
	mov AL,byte[RBP+RSI]	; take  byte of string
	sub AL,30H	; subtract 0 from character

	mul ECX			; mul EAX with weight of digit
 			        ; accoRDIng to byte in string
	add EBX,EAX  ; add to result
	
	dec RSI  		; next higher weight character of string
	mov EAX,10  ; prepare to mul ECX * 10, ECX is weight of char in string
	mul ECX			; mul ECX is AX = AX * ECX. -> mul weight with 10
	mov ECX,EAX ; ECX is the weight, multiplication result was in EAX, so move it to weight
	cmp RSI,-1  ; if RSI = -1, there are no more bytes to convert
	jne ToIntNext	; jump not equal, next character if not 0
	
  ; EBX = representation of entered string

;; Count primes below the number stored in EBX 
	mov EDI,EBX		; limit value, value is in EBX so far
	xor EBP,EBP	  ; clear prime counter

	mov RAX,rsp	; clear data buffer
	mov rsp,RAX

	xor EAX,EAX ; loop index
  
ClearDataBuffer:
	; clear from index 0 to DATABUFFERLEN
	mov qword [DataBuffer+EAX],0H ; set 0
	add EAX,8                     ; increment
	cmp EAX,DATABUFFERLEN         ; if index > DATABUFFERLEN -> done
	jbe  ClearDataBuffer       ; jump below or equal
   
	; check if "PrimesBelow" > 2
	cmp EDI,2	  ; if "PrimesBelow" <= 2 -> no prime numbers
	jbe CountComplete	; "PrimesBelow" is <= 2 -> no prime numbers
	inc EBP      ; number 2 is prime
  
	; start prime count loop
	mov EBX,1		; next instruction is EBX+2

; Looping through numbers in CheckingNumber < PrimesBelow
CountPrimeLoop:		
	add EBX,2		 ; next number to check -> Only check odd numbers
	test byte [DataBuffer+EBX],1   ; if set we have already checked this number
	jnz  CountPrimeLoop            ; jump if zero flag not set --> check next number
	cmp EBX,EDI  ; if CheckingNumber >= PrimesBelow done counting
	jae CountComplete  ; jump above or equal: done
	add EBP,1      ; increment prime counter
	mov ECX,3    ; ECX is factor: set 3 for first number. Get ready for mul

LoopMulNext: 
	mov EAX,EBX  ; Get CheckingNumber ready for mul
	mul ECX      ; mul ECX is EAX = EAX * ECX
	cmp EAX,EDI  ; EAX is "CheckingNumber" * factor, check until >= PrimesBelow
	jae CountPrimeLoop  ; jump above or equal: next number to check for prime
	mov byte [DataBuffer+EAX],1  ; set checked
	add ECX,2                    ; next number for multiplication
	jmp LoopMulNext         ; jump: multiply with next number
  
CountComplete:		
	mov EAX,EBP         ; Copy result to EAX

; Convert the number to Result
	mov RDI,RESULTLEN-1       ; last index of string
	mov byte[Result+RDI],10 	; line feed as last char
	mov ECX,10       ; divider: 10
IntToStr:
	dec RDI          ; next character
	xor EDX,EDX      ; used for division -> clear
	div ECX          ; EDX:EAX / ECX
	add DX,30H		   ; Convert to ASCII 0
	mov byte[Result+RDI],DL 	; put in string
	cmp EAX,0        ; if quotient is 0, conversion is done
	jne IntToStr
  
; Print result to console
	mov EAX,4 ; Specify sys_write call
	mov EBX,1 ; Specify File Descriptor 1: Standard output
	mov ECX,Result  ; ecx: offset of the start of the message
    add ECX,EDI           ; message start is here, as string is RIGHT justified in buffer
	mov EDX,RESULTLEN ; Pass the length of the message
    sub EDX,EDI             ; edi is the first valid index of the message -> length = RESULTSTRINGLEN - edi
	int 80H ; Make kernel call
	jmp Exit		; we're done

Exit:
	mov RAX, 60 ; Code for exit
	mov RDI, 0 ; Return a code of zero
	syscall ; Make kernel call
