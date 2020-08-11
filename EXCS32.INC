;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
exceptions:
        ASSUME  DS:txdata
	; rellenamos el texto de salida con los valores de debugging
        push    ds ebx edi ecx esi ebp edx
        mov     ds,cs:D16_sel
	; n£mero de excepci¢n
	mov     dl,al   ; gate number
	mov     edi,offset exception_number
	xor     cl,cl
	call    reg_to_hex
        ; ponemos los valores de EDX,EBP,ESI,ECX,EDI y EBX
        ASSUME  DS:txcode32
        mov     ebx,6
        pon_registros:
                pop     eax
                mov     edi,cs:[ebx*4+reg_offsets-4]
                mov     cl,2
                push    ebx
                call    reg_to_hex
                pop     ebx
        dec     ebx
        jnz     pon_registros
        ASSUME  DS:txdata

        ; valor de DS
	pop     eax     ; pq estamos en un segmento de 32 bits
	push    eax
	mov     edi,offset debug_ds
	mov     cl,1
	call    reg_to_hex
	pop     ebx
	mov     edi,offset desc_ds
	call    print_desc
	; valor de EAX
	pop     eax
        mov     edi,offset debug_eax
        mov     cl,2    ; 32 bits
        call    reg_to_hex
	; valor de ES ( lo hacemos ahora para poder usar ES sin "popearlo" )
	mov     ax,es
	mov     edi,offset debug_es
	mov     cl,1
	call    reg_to_hex
	mov     bx,es
	mov     edi,offset desc_es
	call    print_desc
        ; si estamos bajo DPMI, tenemos CS:EIP de retorno en pila y no nos interesa
        test    system_type,2
        jz      no_tenemos_el_gusto
                add     esp,3*4
                jmp     no_error_code
        no_tenemos_el_gusto:
	; volcamos el opcode causante del error
	cmp     dl,14           ; s¢lo las int's 9-13 tendr n 'error code' en pila
	ja      no_error_code
	cmp     dl,9
	jb      no_error_code
		pop     eax     ; info de debugging de la CPU
	no_error_code:
	pop     eax     ; EIP
	pop     ebx     ; CS
	push    ebx     
	push    eax
        mov     es,cs:D16_SEL
        test    system_type,2
        jz      normal_gdt
                push    eax
                mov     ax,0Bh
                mov     edi,offset gdt
                int     31h
                jc      invalid_code_selector
                xor     ebx,ebx
                pop     eax
                jmp     continua_volcado
        normal_gdt:
        ; vamos a hacerlo con el selector de 4Gb por si el de c¢digo es s¢lo ejecutable
	movzx   ebx,bx          ; selector c¢digo
	and     bl,11111000b    ; limpiamos el requested PL y nos queda BX = descriptor * 8
        ASSUME  ES:txdata
        continua_volcado:
        mov     cl,byte ptr es:[ebx+7+gdt] ; CL = bits 24-31 de la base
	shl     ecx,24
        mov     ebx,dword ptr es:[ebx+2+gdt] ; bits 0-23 de la base
	and     ebx,(1 SHL 24)-1        ; limpiamos los 8 bits superiores
	or      ebx,ecx                 ; formamos la base completa
	add     ebx,eax                 ; le sumamos el OFFSET
        mov     es,cs:D32_SEL
        mov     edi,offset crash_code
        mov     ecx,8
	vuelca_code:
                mov     al,es:[ebx]
		call    bin_to_hex
		mov     ds:[edi],ax
                add     edi,3
		inc     ebx
        loop    vuelca_code

        invalid_code_selector:
        ; valor de EIP
	pop     eax     ; EIP
	mov     cl,2    
	mov     edi,offset debug_eip
	call    reg_to_hex
	; valor de CS
	pop     eax     ; CS
	push    eax
	mov     cl,1
	mov     edi,offset debug_cs
	call    reg_to_hex
	mov     eax,dword ptr debug_cs
	mov     dword ptr debug_cs1,eax
	pop     ebx
	mov     edi,offset desc_cs
	call    print_desc
        ; valor de FS
        mov     ax,fs
	mov     edi,offset debug_fs
	mov     cl,1
	call    reg_to_hex
	mov     bx,fs
	mov     edi,offset desc_fs
	call    print_desc
	; valor de GS
	mov     ax,gs
	mov     edi,offset debug_gs
	mov     cl,1
	call    reg_to_hex
	mov     bx,gs
	mov     edi,offset desc_gs
	call    print_desc
	; valor de SS
        test    system_type,2
        jz      no_ss_de_pila
                mov     eax,ss:[esp+2*4]
                jmp     ss_de_pila
        no_ss_de_pila:
	mov     ax,ss
        ss_de_pila:
	mov     cl,1
	mov     edi,offset debug_ss
        push    eax
	call    reg_to_hex
        pop     ebx
	mov     eax,dword ptr debug_ss
	mov     dword ptr debug_ss0,eax
	mov     edi,offset desc_ss
	call    print_desc
        ; valor de los EFLAGS
	pop     eax
	push    eax
	mov     cl,2
	mov     edi,offset debug_flags
	call    reg_to_hex
	; valor de ESP antes de la excepci¢n
	mov     eax,esp
        add     eax,4   ; EFLAGS
        test    system_type,2
        jz      normal_esp
                mov     eax,ss:[esp+1*4]
        normal_esp:
	mov     cl,2
	mov     edi,offset debug_esp
	call    reg_to_hex
	mov     eax,dword ptr debug_esp
	mov     dword ptr debug_esp0,eax
	mov     eax,dword ptr debug_esp+4
	mov     dword ptr debug_esp0+4,eax
	; volcamos la pila
        mov     esi,3
	mov     ebp,4
	mov     edi,offset stack_cont1
        mov     ax,ss
        mov     es,ax
        add     ebp,esp
        test    system_type,2
        jz      vuelca_pila
        mov     ax,ss:[esp+2*4]
        mov     ebp,ss:[esp+1*4]
        mov     es,ax
	vuelca_pila:        
                mov     eax,es:[ebp]
		mov     cl,2
		push    ebp
		call    reg_to_hex
		pop     ebp
		add     ebp,4
		add     edi,9
        dec     esi
	jnz     vuelca_pila
	; vamos a rellenar el campo de info sobre el error
	movzx   esi,dl
	shl     esi,1   ; * 2
	movzx   esi,word ptr [esi + error_string_table]
	mov     edi,offset error_string
        xor     ecx,ecx
	put_err_strn:
		mov     al,[esi]
		or      al,al
		jz      end_put
		mov     [edi],al
		inc     edi
		inc     esi
        dec     ecx 
	jnz     put_err_strn
	end_put:

        ; hacemos un acknowledge a los PIC por si la excepci¢n ocurri¢ en
        ; una IRQ
        mov     al,20h          ; acknowledge
        out     20h,al          ; master PIC
        out     0A0h,al         ; slave PIC

        test    system_type,2
        jnz     dpmi_exception_exit

	; vamos a modificar el salto para que salga la info de debugging
        mov     ds,cs:D32_SEL
	mov     ebx,txcode
	shl     ebx,4
        ASSUME  DS:txcode
        mov     word ptr [ebx+saltito+1],offset debug_info
        mov     word ptr [ebx+saltito+3],txcode

        mov     ds,cs:D16_SEL
        mov     es,cs:D32_SEL
        push    dword ptr cs:C16_SEL
        pushd   ret_real
        iretd

ASSUME  DS:txdata
reg_offsets     dd offset debug_ebx,offset debug_edi,offset debug_ecx
                dd offset debug_esi,offset debug_ebp,offset debug_edx

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

dpmi_exception_exit:
; DS = D16_SEL

        popfd
        pop     ebx
        pop     eax
        cli
        mov     ss,ax
        mov     esp,ebx

        inc     dpmi_exception_flag
        pushfd
        push    dword ptr cs:C32_SEL
        pushd   salida_dpmi
        iretd

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

print_desc      proc
; INPUT  -> BX = selector ; DS:EDI = direcci¢n de destino
        test    system_type,2
        jz      normal_sel
                push    es edi
                mov     ax,0Bh
                mov     es,cs:D16_SEL
                mov     edi,offset gdt
                int     31h
                pop     edi es
                jnc     sel_ok
                        push    es
                        mov     es,cs:D16_SEL                                
                        mov     ecx,7*3+2
                        mov     esi,offset invalid_sel_msg
                        cld
                        rep     movsb
                        pop     es
                        ret
                sel_ok:
                xor     ebx,ebx
        normal_sel:
	movzx   ebx,bx
;        and     bl,11111000b
        shr     ebx,3
        cmp     bx,NUM_DESCS+FREE_DESCS
        jae     no_volcado_desc
        cmp     bx,NUM_DESCS
        jb      volcado_desc
        ASSUME  DS:txdata
        cmp     byte ptr [ebx+free_descs_handles-NUM_DESCS],0
        jz      no_volcado_desc
        volcado_desc:
        shl     ebx,3
	mov     cl,8
	vuelca_desc:
                mov     al,byte ptr [ebx+gdt]
		call    bin_to_hex
		mov     [edi],ax
		add     edi,3
		inc     ebx
	dec cl
	jnz vuelca_desc
        no_volcado_desc:
	ret
print_desc      endp

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

reg_to_hex      proc
; INPUT -> EAX = number to convert ; DS:EDI -> destino 
;          CL = 8/16/32 bit register (2=EAX ; 1=AX ; 0=AL)

	xor     bl,bl
	or      cl,cl
	jz      es_al
	cmp     cl,1
	jz      es_ax
	cmp     cl,2
	jz      es_eax        
	es_al:                
		mov     cl,1
		xor     ebp,ebp
		jmp     convert
	es_ax:
		mov     cl,2
		mov     ebp,2
		jmp     convert
	es_eax:
		mov     cl,4
		mov     ebp,6
	convert:
		push    ecx
		mov     cl,bl
		push    eax
		shr     eax,cl
		call    bin_to_hex
		mov     [edi+ebp],ax
		pop     eax
		pop     ecx
		sub     ebp,2
		add     bl,8
	dec     cl
	jnz     convert
	ret
reg_to_hex      endp

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

bin_to_hex      proc
; INPUT  -> AL = byte to convert
; OUTPUT -> AX = hex value
	
	mov     ah,al
	and     ah,0fh
	cmp     ah,9 
	jbe     cifra0
		add     ah,'A'-'9'-1
	cifra0:
	add     ah,'0'
	shr     al,4
	cmp     al,9
	jbe     cifra1
		add     al,'A'-'9'-1
	cifra1:
	add     al,'0'
	ret

bin_to_hex      endp

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
