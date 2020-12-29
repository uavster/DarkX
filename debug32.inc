debug32_move_cursor proc
; INPUT ->	BH = row (or -1 to leave as is)
;			BL = column (or -1 to leave as is)
	ASSUME DS:TXDATA
	pushfd
	push ds
	mov ds,cs:D16_SEL
	cmp bl,-1
	jz leave_col_as_is32
		mov debug_col,bl
	leave_col_as_is32:
	cmp bh,-1
	jz leave_row_as_is32
		mov debug_row,bh
	leave_row_as_is32:
	pop ds
	popfd
	ret
endp

debug32_reg	proc
; INPUT -> 	EAX = number to convert
if DEBUG_ON
	pushfd
	push ds es
	pushad
	ASSUME DS:TXDATA, ES:TXDATA
	mov ds,cs:D16_SEL
	xor	edi,edi
	mov edi,offset debug_tmp
	mov cl,2
	push ds edi
	call reg_to_hex
	pop esi ds
	mov es,cs:D32_SEL
	mov edi,0b8000h
	movzx eax,debug_row
	mov ebx,eax
	shl eax,7
	shl ebx,5
	add eax,ebx
	add edi,eax
	movzx eax,debug_col
	shl eax,1
	add edi,eax
	mov ecx,8
	cld
	mov ah,1eh
	print_debug32:
		lodsb		
		stosw
		inc debug_col
		cmp debug_col,80
		jnz in_col32
			mov debug_col,0
			inc debug_row
		in_col32:
	loop print_debug32
	popad
	pop es ds
	popfd
endif
	ret
endp

debug32_char proc
; INPUT -> AL = Character
if DEBUG_ON
	pushfd
	push ds es
	pushad
	ASSUME DS:TXDATA
	mov ds,cs:D16_SEL
	mov es,cs:D32_SEL
	mov edi,0b8000h
	movzx ecx,debug_row
	mov ebx,ecx
	shl ecx,7
	shl ebx,5
	add ecx,ebx
	add edi,ecx
	movzx ecx,debug_col
	shl ecx,1
	add edi,ecx
	mov ecx,8
	mov ah,1eh	
	stosw
	inc debug_col
	cmp debug_col,80
	jnz in_col_char32
		mov debug_col,0
		inc debug_row
	in_col_char32:
	popad
	pop es ds
	popfd
endif
	ret
endp