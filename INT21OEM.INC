;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; INT 21h Standard DOS Services
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
tx_int21h:        
                cmp     ah,3Fh                  ; read from file
		jz      read_file
                cmp     ah,40h                  ; write to file
		jz      write_file
                cmp     ah,3Dh                  ; open existing file
                jz      generic_file_service
                cmp     ah,3Ch                  ; create file
                jz      generic_file_service
                cmp     ah,39h                  ; create subdirectory
                jz      generic_file_service
                cmp     ah,3Ah                  ; remove subdirectory 
                jz      generic_file_service
                cmp     ah,3Bh                  ; change subdirectory
                jz      generic_file_service
                cmp     ah,41h                  ; delete file
                jz      generic_file_service
                cmp     ax,4300h                ; get file attributes
                jz      generic_file_service
                cmp     ax,4301h                ; set file attributes
                jz      generic_file_service
                cmp     ah,4Eh                  ; find first matching file
                jz      generic_file_service
                cmp     ah,5Bh                  ; create new file
                jz      generic_file_service

                cmp     ah,9                    ; write to standard output
		je      print_string
                cmp     ah,62h                  ; get PSP selector
                jz      get_psp_sel
                cmp     ah,4Ch                  ; end program
		je      fin_programa
		push    ds
		mov     ds,cs:D16_SEL
		ASSUME  DS:txdata
		test    system_type,2
		pop     ds
		jnz     dpmi_int21_pass
		push    eax        ; si no, la tratamos con el handler normal
		mov     al,21h
		jmp     real_int

		dpmi_int21_pass:
		pushfd
		call    fword ptr cs:[idt+21h*6]
		iretd

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; DOS Program Termination Service
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
fin_programa:
		add     esp,3*4
		pushfd
		push    dword ptr cs:C32_sel
		pushd   salida
		iretd

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Common DOS RM Call
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
dos_int         proc
; INPUT -> BP = DS/ES value ; Registers as needed by INT 21H
	push    es
	push    ss
	pop     es
	mov		edi,0	; Don't use XOR to avoid changing flags
	push    edi
	push    edi
	push    edi
	push    bp
	push    bp
        db      66h             ; <----- para WORD en segmento de 32 bits
        pushf
	pushad
	mov     edi,esp
        mov     ax,300h
        mov     bl,21h
	xor     ecx,ecx
	int     31h
	popad
	popf
	lea     esp,[esp+4*4]   ; as¡ no modificamos los flags
	pop     es
	ret
dos_int         endp

copy_file_name  proc
; INPUT -> DS:EDX -> ASCIIZ string
        push    edx es edi ecx eax 
	; copiamos el nombre del archivo a 'print_buffer'
	mov     es,cs:D32_SEL
	mov     edi,txcode32
	shl     edi,4
	add     edi,offset print_buffer
	mov     ecx,128
	copy_name:
		mov     al,[edx]
		mov     es:[edi],al
		or      al,al
		jz      end_of_name
		inc     edi
		inc     edx
	loop    copy_name
	end_of_name:
        pop     eax ecx edi es edx
	ret
copy_file_name  endp

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; DOS Write to Standard Device Service
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
print_string:
		pushad
		push    es
		mov     esi,edx
		print_another_piece:
		mov     es,cs:D32_SEL
		mov     edi,txcode32
		shl     edi,4
		add     edi,offset print_buffer
		mov     ecx,80*25+1
		copy_string:
			cmp     byte ptr [esi],'$'
			je      string_copied
			movsb
		loop    copy_string
		mov     byte ptr es:[edi],'$'
		push    edi
		mov     ah,9
		mov     edx,offset print_buffer
		mov     ebp,txcode32
		call    dos_int
		pop     edi
		jmp     print_another_piece
		string_copied:
		mov     byte ptr es:[edi],'$'
		mov     ah,9
		mov     edx,offset print_buffer
		mov     ebp,txcode32
		call    dos_int
		pop     es
		popad
		iretd

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Disk Services
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
generic_file_service:
; Translates DS:EDX to DS:DX in DOS memory passing/returning all registers
; to/from DOS
	pushad
	push    es
	call    copy_file_name
	mov     edx,offset print_buffer
	mov     ebp,txcode32
	call    dos_int        
	pushfd
	pop     ecx
	and     cl,1
	and     byte ptr ss:[esp+11*4],0FEh
	or      byte ptr ss:[esp+11*4],cl
	mov     ss:[esp+8*4],eax
	pop     es
	popad
	iretd

read_file:
	and     byte ptr ss:[esp+2*4],0FEh
	pushad
	push    es ds
	push    ds
	pop     es
	push    ecx
	mov     ds,cs:D32_SEL
	mov     edi,edx
	xor     eax,eax         ; INICIO -> 0 bytes le¡dos
	shr     ecx,15          ; 32Kb
	or      ecx,ecx
	jz      dont_read_blocks
	read_32K_blocks:
		push    eax ecx
		mov     cx,8000h
		call    read_block
		pop     ecx
		jc      read_error
		cmp     eax,8000h
		jnz     no_resto3
		pop     eax
		add     eax,8000h
	loop    read_32K_blocks
	dont_read_blocks:
	pop     ecx
	push    ecx
	and     ecx,7FFFh
	or      ecx,ecx
	jz      no_resto2
	push    eax
	call    read_block
	jc      read_error
	pop     ebx
	add     eax,ebx
	no_resto2:
	pop     ecx
	pop     ds es
	mov     ss:[esp+7*4],eax
	popad
	iretd

	no_resto3:
	pop     edx
	add     eax,edx
;        pop     ecx
	jmp     no_resto2

	read_error:
	pop     ebx
	or      byte ptr ss:[esp+13*4],1
	jmp     no_resto2

read_block      proc
	mov     edx,offset disk_buffer
	mov     ebp,disk
	mov     ax,3F00h
	push    edi
	call    dos_int
	pop     edi
	jc      no_resto
	movzx   eax,ax
	mov     esi,disk
	shl     esi,4
	add     esi,offset disk_buffer
	mov     ecx,eax
	shr     ecx,2
	cld
	rep     movsd
	mov     ecx,eax
	and     ecx,3
	or      ecx,ecx
	jz      no_resto
	rep     movsb
	no_resto:
	ret
read_block      endp

write_file:
	and     byte ptr ss:[esp+2*4],0FEh
	pushad
	push    es ds
	push    ecx
	mov     es,cs:D32_SEL
	mov     esi,edx
	xor     eax,eax         ; INICIO -> 0 bytes le¡dos
	shr     ecx,15          ; 32Kb
	or      ecx,ecx
	jz      dont_write_blocks
	write_32K_blocks:
		push    eax ecx
		mov     ecx,8000h
		call    write_block
		pop     ecx
		jc      write_error
		cmp     eax,8000h
		jnz     no_resto5
		pop     eax
		add     eax,8000h
	loop    write_32K_blocks
	dont_write_blocks:
	pop     ecx
	push    ecx
	and     ecx,7FFFh
	or      ecx,ecx
	jz      no_resto4
	push    eax
	call    write_block
	jc      write_error
	pop     ebx
	add     eax,ebx
	no_resto4:
	pop     ecx
	pop     ds es
	mov     ss:[esp+7*4],eax
	popad
	iretd

	no_resto5:
	pop     edx
	add     eax,edx
	jmp     no_resto4

	write_error:
	pop     ebx
	or      byte ptr ss:[esp+13*4],1
	jmp     no_resto4

write_block      proc
	mov     edi,disk
	shl     edi,4
	add     edi,offset disk_buffer
	push    ecx
	shr     ecx,2
	cld
	rep     movsd
	pop     eax
	mov     ecx,eax
	and     ecx,3
	rep     movsb
	mov     edx,offset disk_buffer
	mov     ecx,eax
	mov     ebp,disk
	mov     ah,40h
	call    dos_int
	movzx   eax,ax
	ret
write_block      endp

get_psp_sel:
        push    ds
        mov     ds,cs:D16_SEL
        mov     bx,psp_selector
        pop     ds
        iretd
