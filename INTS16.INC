;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Standard DPMI 0.9 Real Mode Call and Interrupt execution with segment pass
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
dpmi_rm_int:
        cli

        ASSUME  SS:txdata
        mov     ax,txdata
        mov     ss,ax
        mov     sp,offset dpmi_rm_call_struc+2Ah
        pop     word ptr cs:rm_proc_address
        pop     word ptr cs:rm_proc_address+2
        pop     cs:stack_frame
        pop     cs:stack_frame+2        
        cmp     dword ptr cs:stack_frame,0
        jnz     need_a_stack
        mov     cs:stack_frame,size txpr
        mov     cs:stack_frame+2,txpila_real
        need_a_stack:
        mov     ax,cs:bytes_in_stack
        sub     cs:stack_frame,ax
        mov     eax,ss:pm_eax
        mov     cs:int_or_call,ax
        cmp     ax,300h
        jnz     pre_rm_call

        ; reajustamos la pila para int en modo real
        xor     bx,bx
        mov     cs:trash,ds
        mov     ds,bx
        mov     bl,byte ptr ss:pm_ebx
        shl     bx,2
        mov     eax,ds:[bx]
        mov     cs:rm_proc_address,eax
        mov     ds,cs:trash

pre_rm_call:
        mov     sp,offset dpmi_rm_call_struc
        popad
        pop     cs:trash        ; trash = FLAGS para no cambiarlos con el CMP
        pop     es
        pop     ds
        pop     fs
        pop     gs
        lss     sp,dword ptr cs:stack_frame
        cmp     cs:int_or_call,301h
        jz      no_flag_save
        push    cs:trash        ; para formar un stack frame de interrupci¢n
        no_flag_save:
        call    dword ptr cs:rm_proc_address

        ; copiamos los segmentos a la estructura
        ASSUME  DS:txdata
        push    ds
        push    txdata
        pop     ds
        mov     real_es,es
        mov     real_fs,fs
        mov     real_gs,gs
        pop     real_ds

        ; restablecemos la pila por defecto por si ha habido int31h/ax=30xh
        mov     cs:stack_frame,size txpr
        mov     cs:stack_frame+2,txpila_real

	jmp     set_pm          ; SET_PM nos grabar  el estado

int_or_call     dw ?
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Real Mode Interrupt execution without segment pass
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
ASSUME  DS:txdata
rm_int:
        ; reajustamos la pila para int en modo real
        lss     sp,dword ptr cs:stack_frame
        xor     bx,bx
        mov     cs:trash,ds
        mov     ds,bx
        mov     bl,cs:int_num
        shl     bx,2
        mov     eax,[bx]
        mov     cs:rm_proc_address,eax
        mov     ds,cs:trash

        push    ds

	mov     ax,txdata
	mov     ds,ax

        ; recuperamos los registros
	mov     eax,pm_eax
	mov     ebx,pm_ebx
	mov     ecx,pm_ecx
	mov     edx,pm_edx
	mov     esi,pm_esi
	mov     edi,pm_edi
	mov     ebp,pm_ebp
	push    pm_eflags
        popfd
        pop     ds

        pushf           ; para formar un stack frame de interrupci¢n
        call    dword ptr cs:rm_proc_address

	jmp     set_pm          ; SET_PM nos grabar  el estado

int_num         db ?
trash           dw ?
stack_frame     dw size txpr,?
bytes_in_stack  dw 0
rm_proc_address dd ?
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
irq16:
        lss     sp,dword ptr cs:stack_frame
        sub     sp,(RM_STACKSIZE/2)*4   ; por si pillamos un proc real entrando
        pushf                   ; stack frame de interrupci¢n
        call    dword ptr cs:rm_proc_address
        mov     ax,txdata
        mov     ds,ax
        jmp     set_pm_address
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
