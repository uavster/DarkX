.386p
.model flat
.stack 400h
.code
dd      0

Trm_reg         STRUC
        rm_edi          dd ?
        rm_esi          dd ?
        rm_ebp          dd ?
                        dd ?
        rm_ebx          dd ?
        rm_edx          dd ?
        rm_ecx          dd ?
        rm_eax          dd ?
        rm_flags        dw ?
        rm_es           dw ?
        rm_ds           dw ?
        rm_fs           dw ?
        rm_gs           dw ?
        rm_ip           dw ?
        rm_cs           dw ?
        rm_sp           dw ?
        rm_ss           dw ?
ENDS

ErrorExit MACRO error_msg
	local no_error, err_msg
	jnc		no_error
			mov		edx,offset err_msg
			jmp		error_handler
err_msg		db error_msg
	no_error:
ENDM

MESSAGE_LEN	EQU offset end_message-offset message+1

start:  jmp hello
db 'WATCOM'
hello:
	push	ds
	pop		es
	; Get INT 21h address
	mov		ax,200h
	mov		bl,21h
	int		31h
	ErrorExit "Unable to get int 21h address."
	mov		edi,offset rm_regs
	mov		[edi.rm_cs],cx
	mov		[edi.rm_ip],dx
	; Allocate 100h bytes of DOS memory to copy string
	mov		ax,100h
	mov		bx,MESSAGE_LEN
	shr		bx,4	; paragraphs of 16 bytes each
	inc		bx		; At least one
	int		31h
	ErrorExit "Unable to allocate DOS memory."
	mov		edi,offset rm_regs
	mov		[edi.rm_ds],ax
	mov		[edi.rm_edx],0
	; Copy string to DOS memory
	movzx	edi,ax	
	shl		edi,4
	mov		esi,offset message
	mov		ecx,MESSAGE_LEN
	cld
	rep		movsb
	; Call real mode proc with iret frame
	mov		ax,302h
	xor		bh,bh
	mov		cx,0
	mov		edi,offset rm_regs
	mov		[edi.rm_eax],0900h
	mov		[edi.rm_ss],0
	mov		[edi.rm_sp],0
	mov		[edi.rm_flags],0
	int		31h
	ErrorExit "Error calling real mode procedure with iret frame."
	; End program
	end_program:
	mov		ah,4Ch
	int		21h
	
	error_handler:
	mov		ah,9
	int		21h
	jmp		end_program
	
rm_regs	Trm_reg<?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?>
message	db "Hello world"
end_message db "$"

end start

