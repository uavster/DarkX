reg_to_hex16      proc
; INPUT -> EAX = number to convert ; DS:EDI -> destino 
;          CL = 8/16/32 bit register (2=EAX ; 1=AX ; 0=AL)

	xor     bl,bl
	or      cl,cl
	jz      es_al16
	cmp     cl,1
	jz      es_ax16
	cmp     cl,2
	jz      es_eax16        
	es_al16:                
		mov     cl,1
		xor     ebp,ebp
		jmp     convert16
	es_ax16:
		mov     cl,2
		mov     ebp,2
		jmp     convert16
	es_eax16:
		mov     cl,4
		mov     ebp,6
	convert16:
		push    ecx
		mov     cl,bl
		push    eax
		shr     eax,cl
		call    bin_to_hex16
		mov     [edi+ebp],ax
		pop     eax
		pop     ecx
		sub     ebp,2
		add     bl,8
	dec     cl
	jnz     convert16
	ret
reg_to_hex16      endp

;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�

bin_to_hex16      proc
; INPUT  -> AL = byte to convert
; OUTPUT -> AX = hex value
	
	mov     ah,al
	and     ah,0fh
	cmp     ah,9 
	jbe     cifra016
		add     ah,'A'-'9'-1
	cifra016:
	add     ah,'0'
	shr     al,4
	cmp     al,9
	jbe     cifra116
		add     al,'A'-'9'-1
	cifra116:
	add     al,'0'
	ret

bin_to_hex16      endp

;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�

debug16_move_cursor proc
; INPUT ->	BH = row (or -1 to leave as is)
;			BL = column (or -1 to leave as is)
	ASSUME DS:TXDATA
	pushf
	push ds ax
	mov ax,txdata
	mov ds,ax
	cmp bl,-1
	jz leave_col_as_is16
		mov debug_col,bl
	leave_col_as_is16:
	cmp bh,-1
	jz leave_row_as_is16
		mov debug_row,bh
	leave_row_as_is16:
	pop ax ds
	popf
	ret
endp

debug16_reg	proc
; INPUT -> 	EAX = number to convert
if DEBUG_ON
	pushfd
	push ds es
	pushad
	ASSUME DS:TXDATA, ES:TXDATA
	mov bx,txdata
	mov ds,bx
	mov edi,offset debug_tmp
	mov cl,2
	push ds edi
	call reg_to_hex16
	pop esi ds
	mov ax,0b800h
	mov es,ax
	movzx eax,debug_row
	mov ebx,eax
	shl eax,7
	shl ebx,5
	add eax,ebx
	mov edi,eax
	movzx eax,debug_col
	shl eax,1
	add edi,eax
	mov ecx,8
	cld
	mov ah,3eh
	print_debug16:
		lodsb		
		stosw
		inc debug_col
		cmp debug_col,80
		jnz in_col16
			mov debug_col,0
			inc debug_row
			cmp debug_row,40
			jnz in_col16
				mov debug_row,0
		in_col16:
	loop print_debug16
	popad
	pop es ds
	popfd
endif
	ret
endp

debug16_char proc
; INPUT -> AL = Character
if DEBUG_ON
	pushfd
	push ds es
	pushad
	ASSUME DS:TXDATA
	mov bx,txdata
	mov ds,bx
	mov bx,0b800h
	mov es,bx
	movzx ecx,debug_row
	mov ebx,ecx
	shl ecx,7
	shl ebx,5
	add ecx,ebx
	mov edi,ecx
	movzx ecx,debug_col
	shl ecx,1
	add edi,ecx
	mov ecx,8
	mov ah,3eh	
	stosw
	inc debug_col
	cmp debug_col,80
	jnz in_col_char16
		mov debug_col,0
		inc debug_row
			cmp debug_row,40
			jnz in_col_char16
				mov debug_row,0
	in_col_char16:
	popad
	pop es ds
	popfd
endif
	ret
endp