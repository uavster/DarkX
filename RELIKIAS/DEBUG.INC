rm_print_char proc
	mov ah,2
	mov dl,al
	int 21h
	ret
endp

rm_print_reg proc
	mov bx,ax
	mov cx,4
	print_reg_loop:
	rol bx,4
	push bx cx
	mov al,bl
	and al,0fh
	cmp al,9
	ja alpha
	add al,'0'
	jmp num_cont
	alpha:
	add al,'A'-10
	num_cont:
	call rm_print_char
	pop cx bx
	loop print_reg_loop
	ret
endp

rm_print_address proc
	push ax
	mov ax,bx
	call rm_print_reg
	mov al,':'
	call rm_print_char
	pop ax
	call rm_print_reg
	ret
endp

rm_print_interrupt_vector proc
	xor bh,bh
	xor	ax,ax
	mov fs,ax
	shl bx,2
	mov ax,fs:[bx]
	mov bx,fs:[bx+2]
	call rm_print_address
	ret
endp

;	mov bl,8
;	call rm_print_interrupt_vector
;	mov al,10
;	call rm_print_char
;	xor ah,ah
;	int 16h
