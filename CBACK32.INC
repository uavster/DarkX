callback_pm:
        cli
        ; preparamos los datos de entrada para el call-back
        mov     ax,cs:D32_SEL
        mov     ds,ax
        mov     ss,ax
        mov     ebx,txcode
        shl     ebx,4
        ASSUME  DS:txcode
        ; puntero a la estructura de call-backs
        movzx   eax,word ptr [ebx+callback_code]
        ; DS:ESI -> real mode stack frame (SS:SP)
        movzx   ecx,word ptr [ebx+pila_real]   ; real mode SP
        movzx   esi,word ptr [ebx+pila_real+2] ; real mode SS
        mov     edx,esi
        mov     ebp,ecx
        shl     esi,4
        add     esi,ecx
        ; ES:EDI -> real mode call structure
        mov     ecx,txdata
        shl     ecx,4
        ASSUME  DS:txdata
        mov     edi,[ecx+eax+callback_data.rm_call_struc.loffset]   ; real mode
        mov     es,[ecx+eax+callback_data.rm_call_struc.selector]   ; call struc.
        ; SS:SP de real y CS:IP del call-back a la estructura
        mov     es:[edi.rm_sp],bp
        mov     es:[edi.rm_ss],dx
        mov     dx,[ecx+eax+callback_data.rm_entry_point]
        mov     es:[edi.rm_ip],dx
        mov     es:[edi.rm_cs],txcode
        ; SS:ESP -> locked protected mode API stack
;        mov     esp,txpila
;        shl     esp,4
;        add     esp,size tx_pila-4*(PM_STACKSIZE/2)  ; we are not using stack before this
        mov     esp,[ecx+pm_esp]
        mov     ss,[ecx+pm_ss]
        push    ds ebx
        pushfd          ; procedure will return with IRETD
        call    fword ptr [ecx+eax+callback_data.pm_proc_to_call]

        ; ES:EDI -> real mode call structure to restore
        pop     ebx ds
        ; preparamos CS,IP,SS,SP y FLAGS de la estructura para modo real
        ASSUME  DS:txcode
        mov     eax,dword ptr es:[edi.rm_ip]
        mov     [ebx+new_code],eax
        mov     eax,dword ptr es:[edi.rm_sp]
        mov     [ebx+new_stack],eax
        mov     ax,es:[edi.rm_flags]
        mov     [ebx+new_flags],ax

        ; saltamos a modo real de nuevo
        mov     word ptr [ebx+saltito+1],offset cback16_return
        mov     word ptr [ebx+saltito+3],txcode
        ASSUME  DS:txdata
        mov     ds,cs:D16_SEL
        mov     es,cs:D32_SEL
        push    dword ptr cs:C16_SEL
        push    dword ptr ret_real_address
        retf
