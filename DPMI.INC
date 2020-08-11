SELECTORS       equ 7    ; no queremos los 4 del VCPI ni el del PSP
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
dpmi_error:
; DS = D16_SEL 
	; ponemos la real mode call structure
	push    ds
	pop     es
	mov     edi,offset dpmi_rm_call
	mov     byte ptr [edi+1Ch],9
	mov     word ptr [edi+24h],txdata
	mov     word ptr [edi+14h],offset dpmi_init_error
;        mov     word ptr [di+30h],txpila
;        mov     word ptr [di+2Eh],size txpila
	mov     bx,21h
	xor     cx,cx
	mov     ax,300h
	int     31h
	jmp     no_vcpi_end

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

save_dpmi_data  proc
	mov     dx,offset dpmi_32b_error
	test    bl,1
	jz      error_handle
	ASSUME  DS:txdata
	mov     ax,txdata
	mov     ds,ax
	; guardamos el DPMI mode-switch entry point
	mov     word ptr cs:dpmi_mode_switch,di
	mov     word ptr cs:dpmi_mode_switch+2,es
	push    si
	; alojamos la memoria que necesite el DPMI
	pop bx
	call    get_dos_mem
	mov     dpmi_private_buffer,ax
	ret
save_dpmi_data  endp
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

dpmi_init:
		; cargamos en DS el segmento para corregirlo todo
		mov     ax,txdata
		mov     ds,ax

		; PM switch
		mov     ax,1
		mov     es,dpmi_private_buffer
			db      9Ah     ; CALL FAR PTR
dpmi_mode_switch        dd 0
                cli                     ; por si acaso...
		mov     dx,offset dpmi_init_error
		jc      error_handle    ; estaremos todav¡a en real

                ; guardamos el selector del PSP
                mov     psp_selector,es

                ; guardamos el selector del entorno

		; estamos en protegido
		; alojamos descriptores
		xor     ax,ax
		mov     cx,SELECTORS
		int     31h
		jc      dpmi_error
		push    ax
		
		; miramos el incremento de selector
		mov     ax,3
		int     31h

		mov     bp,ax
		pop     bx

		; corregimos los selectores
		mov     ecx,SELECTORS
		ASSUME  DS:txdata
		mov     edi,offset dpmi_sels
		mov     dx,bx
		suma_sels:
			mov     [edi],dx
			add     dx,ax
			add     edi,4
		loop    suma_sels

		; averiguamos el PL actual y lo ponemos en los descriptores
		push    ds
		pop     es
		mov     ax,cs
		lar     dx,ax   ; coge access rights
		and     dh,60h  ; a¡sla PL
		mov     ecx,SELECTORS
		mov     edi,offset gdt+8
		pon_dpl:
			or      es:[edi+5],dh
			mov     ax,0Ch
			int     31h
			jc      dpmi_error
			add     edi,8
			add     bx,bp
                loop    pon_dpl
		; fix-up de selectores
		mov     es,word ptr dpmi_sels+4
		mov     edi,txcode32
		shl     edi,4
		add     edi,offset C32_SEL

		mov     esi,offset dpmi_sels
		mov     ecx,SELECTORS
		; si los copiamos con MOVSD no los copia bien (???)
		copia_sels:
			mov     eax,[esi]
			mov     es:[edi],eax
			add     edi,4
			add     esi,4
		loop copia_sels
		mov     ax,word ptr dpmi_sels

		; SS:ESP = D32_SEL:STACK_PTR
;                mov     ss,dpmi_sels+4
;                mov     esp,stack_ptr
		
		; hacemos el fix-up del salto a protegido antes de cargarnos DS
		mov     word ptr prot_code_address+4,ax
		push    ax

;                ; pedimos las direcciones de los procs. de save/restore
;                mov     ax,305h
;                int     31h
;                mov     word ptr dpmi_save_restore_real,cx
;                mov     word ptr dpmi_save_restore_real+2,bx
;                mov     dword ptr dpmi_save_restore_pm,edi
;                mov     word ptr dpmi_save_restore_pm+4,si
;                ; pedimos las direcciones de switch en raw mode
;                mov     ax,306h
;                int     31h
;                mov     word ptr dpmi_real_to_pm,cx
;                mov     word ptr dpmi_real_to_pm+2,bx
;                mov     dword ptr dpmi_pm_to_real,edi
;                mov     word ptr dpmi_pm_to_real+4,si

		; nos guardamos las excepciones
		xor     bl,bl
		mov     edi,offset dpmi_exc_handlers
		guarda_excs:
			mov     ax,202h
			int     31h
			mov     [edi],edx
			mov     [edi+4],cx
			add     edi,6
			inc     bl
		cmp     bl,15
		jnz guarda_excs
		; ajustamos las excepciones 0,1,3-0Eh
		pop     cx
		push    cx
		xor     bl,bl
		mov     edx,offset interrup_redir
		dpmi_set_excs:
			mov     ax,203h
			int     31h
			jc      dpmi_error
			inc     bl
			add     edx,8
		cmp     bl,15
		jnz     dpmi_set_excs
		mov     edx,dword ptr dpmi_exc_handlers+2*6
		mov     cx,word ptr dpmi_exc_handlers+2*6+4
		mov     ax,203h
		mov     bl,2    ; la 2 no es excepci¢n...
		int     31h
		jc      dpmi_error
	 
		; guardamos las interrupciones
                xor     bl,bl
                mov     edi,txcode32
                shl     edi,4
                guarda_ints:
                        mov     ax,204h
                        int     31h
                        ASSUME  ES:txcode32
                        mov     dword ptr es:[edi+idt],edx
                        mov     word ptr es:[edi+idt+4],cx
                        add     edi,6
                inc bl
                jnz guarda_ints

                ; ponemos las nuevas int's 21h y 31h
		pop     cx
                push    cx
		mov     bl,21h
                mov     edx,offset tx_int21h
		mov     ax,205h
		int     31h
		jc      dpmi_error

                pop     cx
                mov     bl,31h
                mov     edx,offset tx_int31h
                mov     ax,205h
                int     31h
                jc      dpmi_error
                
                ; rellenamos info varia para la funci¢n OEM 3201h
                mov     ax,cs:D32_SEL
                mov     zero_sel,ax
                mov     ax,cs:DATA_SEL
                mov     prog_sel,ax
                mov     ax,cs:ENVRN_SEL
                mov     env_selector,ax
                ASSUME  ES:txdata

                ; y nos vamos al programa
		jmp     fword ptr prot_code_address
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
