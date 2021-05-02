CALLBACK_TOTAL_STACK_SIZE 	EQU 4*(PM_STACKSIZE/2)
CALLBACK_REENTRY_STACK_SIZE	EQU CALLBACK_TOTAL_STACK_SIZE/DPMI_MAX_REENTRIES

callback_pm:
        cli
        ; preparamos los datos de entrada para el call-back
		; Coming from RM, selectors may have invalid values.
		; If they are pushed and poped in an interrupt, for instance,
		; we may get a general protection fault. Set them to a safe value.
        mov     ax,cs:D32_SEL
        mov     ds,ax
		mov		es,ax
		mov		fs,ax
		mov		gs,ax
        mov     ss,ax
        mov     ebx,txcode
        shl     ebx,4
        ASSUME  DS:txcode
        ; puntero a la estructura de call-backs
        movzx   eax,word ptr ds:[ebx+callback_code]
        ; DS:ESI -> real mode stack frame (SS:SP)
        movzx   ecx,word ptr ds:[ebx+pila_real]   	; real mode SP
        movzx   esi,word ptr ds:[ebx+pila_real+2] 	; real mode SS
		mov		bp,word ptr ds:[ebx+flags_real]	; real mode flags
        mov     edx,esi
		shl		edx,16
		mov		dx,cx	; EDX = SS:SP
        shl     esi,4
        add     esi,ecx	; ESI = SS<<4+SP
        ; ES:EDI -> real mode call structure
        mov     ecx,txdata
        shl     ecx,4
        ASSUME  DS:txdata
        mov     edi,ds:[ecx+eax+callback_data.rm_call_struc.loffset]   ; real mode
        mov     es,ds:[ecx+eax+callback_data.rm_call_struc.selector]   ; call struc.
        ; SS:SP de real, CS:IP y flags del call-back a la estructura
		mov		es:[edi.rm_flags],bp
        mov     es:[edi.rm_sp],dx
		shr		edx,16
        mov     es:[edi.rm_ss],dx
        mov     dx,[ecx+eax+callback_data.rm_entry_point]
        mov     es:[edi.rm_ip],dx
        mov     es:[edi.rm_cs],txcode
        ; SS:ESP -> locked protected mode API stack
		; Each callback gets its own stack, up to DPMI_MAX_REENTRIES
		ASSUME DS:txdata
		mov		ds,cs:D16_SEL
		mov		edx,txpila
		shl		edx,4
		add		edx,CALLBACK_TOTAL_STACK_SIZE
		sub		edx,callback_stack_offset
		add		callback_stack_offset,CALLBACK_REENTRY_STACK_SIZE
		mov		esp,edx
		mov		ds,cs:D32_SEL
		ASSUME DS:txcode

        push    ds ebx
        pushfd          ; procedure will return with IRETD
        call    fword ptr [ecx+eax+callback_data.pm_proc_to_call]

        ; ES:EDI -> real mode call structure to restore
        pop     ebx ds
        ; preparamos CS,IP,SS,SP y FLAGS de la estructura para modo real

        ASSUME  DS:txcode
        mov     eax,dword ptr es:[edi.rm_ip]
        mov     ds:[ebx+new_code],eax
        mov     eax,dword ptr es:[edi.rm_sp]
        mov     ds:[ebx+new_stack],eax
        mov     ax,es:[edi.rm_flags]
        mov     ds:[ebx+new_flags],ax

        ; saltamos a modo real de nuevo
        mov     word ptr [ebx+saltito+1],offset cback16_return
        mov     word ptr [ebx+saltito+3],txcode
        ASSUME  DS:txdata
        mov     ds,cs:D16_SEL
        mov     es,cs:D32_SEL
		sub		callback_stack_offset,CALLBACK_REENTRY_STACK_SIZE
        push    dword ptr cs:C16_SEL
        push    dword ptr ret_real_address
        retf
