;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
ret_real_vcpi:
; INPUT -> DS = D16_SEL ; ES = D32_SEL
        cli

        push    es
        pop     ss
		ASSUME	CS:txcode
		movzx	eax,word ptr cs:dpmi_actual_rm_stack_frame+2
		shl		eax,4
		movzx	edi,word ptr cs:dpmi_actual_rm_stack_frame
		add		edi,eax
		movzx	eax,es:[edi.rm_ss]
		shl		eax,4
		movzx	ebx,es:[edi.rm_sp]
		add		ebx,eax
        mov     esp,ebx
        sub     esp,4*4 ; simulamos 'push' de los selectores
        xor     eax,eax
        mov     ax,es:[edi.rm_ss]
		push    eax
        mov     ax,es:[edi.rm_sp]
        push    eax
        pushfd
        ASSUME  ES:TXCODE
        mov     ebx,txcode
        shl     ebx,4
        movzx   eax,word ptr es:[ebx+saltito+3]
        push    eax             ; CS
        movzx   eax,word ptr es:[ebx+saltito+1]
        push    eax             ; EIP
        ASSUME  ES:TXDATA,DS:txdata
        mov     ax,ds
        mov     bx,es
        mov     es,ax
        mov     ds,bx
		
        mov     ax,0DE0Ch
		; If the client does any task switching, it should clear the TS bit
		; in CR0 before making this call. Otherwise, the TS bit will still 
		; be set after the switch to V86 mode, and a floating point coprocessor 
		; instruction in V86 mode will cause an exception 7 trap to the server. 
		clts	; Clear Task-Switched Flag in CR0
        call    fword ptr es:vcpi_entry_point

;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�

set_pm_vcpi:
; DS = TXDATA
        mov     eax,prot_code_address
        mov     vcpi_cs_eip,eax
        mov     ax,0DE0Ch
        mov     esi,txdata
        shl     esi,4
        add     esi,offset vcpi_sys_regs
        int     67h

;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�

save_vcpi_data  MACRO
        mov     ax,txdata
        mov     ds,ax
        mov     set_pm_address,offset set_pm_vcpi
        mov     ret_real_address,offset ret_real_vcpi
ENDM

;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�

vcpi_init       proc
; INPUT -> DS = TXDATA
		ASSUME	DS:txdata
        ; le decimos que redireccionamos las irq's
        mov     ax,0DE0Ah
        int     67h     ; averiguamos la base de las irq's 8-15
        or      ah,ah
        jnz     vcpi_init_error
        mov     cs:irq_base,bl
        mov     ax,0DE0Bh
        mov     bx,NEW_IRQ0_IRQ7_BASE
        cli
        int     67h
        or      ah,ah
        jnz     vcpi_init_error

        ; le pedimos un interface al gestor
        ; necesitamos un buffer de 4Kb por cada 4Mb que haya en el sistema
        mov     ax,0DE02h
        int     67h
		test	ah,ah
        jnz     vcpi_init_error
		; EDX=physical address of highest 4Kb page
        add     edx,4096+3FFFFFh  ; (tama쨚 = direcci줻 즠tima p쟥ina + 4096)+3fffffh para alinear a 4Mb
        and     edx,NOT 3FFFFFh
        push    edx

        shr     edx,14  ; phys.address*4Kb/4Mb/16   (p쟲rafos necesarios)
        mov     bx,dx
        ; queremos adem쟳 4Kb para el directorio de p쟥inas
        ; + 4K por si est� desalineado
        ; + 256 bytes para el TSS (104 m죒imo)
        add     bx,(4096+4096+256)/16
        call    get_dos_mem
		mov		vcpi_page_dir,ax
        add     ax,0FFh         ; lo alineamos a 4Kb (100h 16-byte paragraphs)
        and     ax,0FF00h       ; a la siguiente p쟥ina
        mov     es,ax

        ; inicializamos el directorio de p쟥inas a 0
        xor     di,di
        xor     eax,eax
        mov     cx,1000h/4
        cld
        rep     stosd
        ; mapeamos toda la memoria existente a s� misma
        pop     ecx
        shr     ecx,22          ; ECX = N즡ero de tablas de p쟥inas necesarias
        mov     eax,67h         ; first page dirty,accessed,user,writeable,present
        push    es ds
        push    es
        pop     ds
        xor     si,si
        set_page_dir_entries:
                push    cx
                mov     di,1000h
                mov     cx,1024 ; pages in a page table
                set_page_table_entries:
                        stosd
                        add     eax,1000h
                loop    set_page_table_entries
                mov     cx,es
                push    cx
                add     cx,4096/16      ; ES:1000h -> siguiente tabla
                mov     es,cx
                xor     ecx,ecx
                pop     cx
                ; llenamos la entrada del directorio de p쟥inas
                shl     ecx,4
                add     ecx,1007h       ; siguiente tabla (USER,WRITEABLE,PRESENT)
                mov     [si],ecx
                add     si,4
                pop     cx
        loop    set_page_dir_entries

        ; guardamos la primera entrada libre en el directorio de p쟥inas
        xor     ebx,ebx
        mov     bx,ds
        pop     ds
        shl     ebx,4
        mov     dir_base,ebx
        movzx   esi,si
        add     ebx,esi
        mov     page_dir_base,ebx       ; linear address of first free page dir. entry
        mov     ebx,4096/4              ; total number of entries in directory
        shr     esi,2
        sub     ebx,esi
        mov     page_dir_entries,ebx    ; number of available entries

        ; el espacio para el TSS est� despu굎 de la 즠tima tabla de p쟥inas
        mov     ax,es
        pop     es
        movzx   eax,ax
        shl     eax,4
        add     eax,1000h       ;1000h-7
        mov     word ptr tss_desc+2,ax
        shr     eax,16
        mov     byte ptr tss_desc+4,al

        ; VCPI Get Protected Mode Interface
		; DS:SI -> Descriptors
		; ES:DI -> Client pages
        mov     di,1000h
        mov     ax,0DE01h
        mov     si,offset vcpi_descs    ; DS:SI -> buffer 3 desc.'s
        int     67h		; DOSBOX crashes at this point after running other programs
        or      ah,ah
        jnz     vcpi_init_error
        mov     vcpi_entry_point,ebx

;        xor     ebx,ebx
;        mov     bx,es
;        shl     ebx,4
;        mov     page_base,ebx
;        add     page_base,1000h         ; page table base
;        movzx   edi,di
;        mov     last_free_page,ebx
;        add     last_free_page,2000h    ; + 4Kb (page dir.) + 4Kb (page table)
;        add     ebx,edi
;        mov     first_free_page,ebx
        ; miramos si el directorio de p쟥inas est� paginado a alg즢 sitio
        mov     bx,es
        shr     bx,2
        mov     eax,es:[bx+1000h]
        shr     eax,4
        xor     al,al
        ; preparamos la tabla de p쟥inas
        mov     vcpi_cr3,eax
        ; preparamos la estructura de registros para el salto
        mov     eax,txdata
        shl     eax,4
        add     vcpi_gdtr,eax
        add     vcpi_idtr,eax
        add     vcpi_cs_eip,offset c32_init
        ret

        vcpi_init_error:
		mov     dx,offset vcpi_init_err_msg
		call    printf
		jmp     fin

vcpi_init       endp

;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�

vcpi_uninit		proc
; INPUT -> DS = TXDATA
	pushf
	cli
	; Tell the VCPI server that we're restoring the IRQ base
	xor     bx,bx
	mov     bl,cs:irq_base
	mov     ax,0DE0Bh
	int     67h
	mov		ax,vcpi_page_dir
	test	ax,ax
	jz		no_vcpi_page_dir
			call	free_dos_mem
	no_vcpi_page_dir:
	popf
	ret
vcpi_uninit		endp

;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
