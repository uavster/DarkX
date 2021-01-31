;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Standard DPMI 0.9 Real Mode Call and Interrupt execution with segment pass
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

dpmi_rm_retf:
		; This is called after a PM-to-RM mode change. Therefore, interrupts are disabled,
		; which is what we need as we mess with the stack.
		lss		sp,cs:dpmi_actual_rm_stack_frame		
		mov		bp,ss
		mov		ds,bp
		
		; Set up all registers from passed DPMI structure
        popad
		mov		bp,sp
		sub		bp,size Tpushad_regs
		mov 	es,ds:[bp.rm_es]
		mov		fs,ds:[bp.rm_fs]
		mov		gs,ds:[bp.rm_gs]
		lss		sp,dword ptr ds:[bp.rm_sp]	; Set RM stack

		; Set return address
		push	cs
		push	offset after_rm_call
		
		; Set RM call address
		push	ds:[bp.rm_flags]
		push	ds:[bp.rm_cs]
		push	ds:[bp.rm_ip]
		
		; Recover RM ds and bp
		push	ds:[bp.rm_ds]
		mov		ebp,ds:[bp.rm_ebp]
		pop		ds

		; Call RM proc
		iret
		
		after_rm_call:
		; The target RM procedure must leave the stack as it found it.
		
		; Disable interrupts before messing with the stack
		push	ds bp
		pushf
		cli
		mov		bp,word ptr cs:dpmi_actual_rm_stack_frame
		mov		ds,word ptr cs:dpmi_actual_rm_stack_frame+2
		pop		ds:[bp.rm_flags]
		pop		bp ds
		
		; Copy registers back to DPMI structure
		lss		sp,cs:dpmi_actual_rm_stack_frame
		add		sp,offset rm_ip
		push	gs fs ds es
		sub		sp,2	; jump over flags
		pushad

		; Restore stack in case it's needed for the mode switch
		mov		bp,sp
		lss		sp,dword ptr ss:[bp.rm_sp]
		
		; Back to PM
		jmp     set_pm_dpmi


dpmi_rm_iret:
		; This is called after a PM-to-RM mode change. Therefore, interrupts are disabled,
		; which is what we need as we mess with the stack.
		lss		sp,cs:dpmi_actual_rm_stack_frame		
		mov		bp,ss
		mov		ds,bp
		
		; Set up all registers from passed DPMI structure
        popad
		mov		bp,sp
		sub		bp,size Tpushad_regs
		mov 	es,ds:[bp.rm_es]
		mov		fs,ds:[bp.rm_fs]
		mov		gs,ds:[bp.rm_gs]
		lss		sp,dword ptr ds:[bp.rm_sp]	; Set RM stack

		pushf
		push	cs
		push	offset after_rm_int		; Return address
		
		and		ds:[bp.rm_flags],not 0300h
		push	ds:[bp.rm_flags]
		push	ds:[bp.rm_cs]
		push	ds:[bp.rm_ip]			; RM call address
		
		; Recover RM ds, bp and flags
		push	ds:[bp.rm_ds]
		mov		ebp,ds:[bp.rm_ebp]
		pop		ds

		; Call RM proc
		iret
		
		after_rm_int:
		; The target RM procedure must leave the stack as it found it.
		
		; Disable interrupts before messing with the stack
		push	ds bp
		pushf
		cli
		mov		bp,word ptr cs:dpmi_actual_rm_stack_frame
		mov		ds,word ptr cs:dpmi_actual_rm_stack_frame+2
		pop		ds:[bp.rm_flags]
		pop		bp ds
		
		; Copy registers back to DPMI structure
		lss		sp,cs:dpmi_actual_rm_stack_frame
		add		sp,offset rm_ip
		push	gs fs ds es
		sub		sp,2	; jump over flags
		pushad

		; Restore stack in case it's needed for the mode switch
		mov		bp,sp
		lss		sp,dword ptr ss:[bp.rm_sp]
		
		; Back to PM
		jmp     set_pm_dpmi


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Real Mode Interrupt execution without segment pass
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
ASSUME  DS:txdata
rm_int:
        ; reajustamos la pila para int en modo real
        lss     sp,dword ptr cs:stack_frame
        xor     bx,bx
;        mov     cs:trash,ds
        mov     ds,bx
        mov     bl,cs:int_num
        shl     bx,2
        mov     eax,[bx]
        mov     cs:rm_proc_address,eax
;        mov     ds,cs:trash

;        push    ds

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
		push    word ptr pm_eflags
        popf
;        pop     ds

        pushf           ; para formar un stack frame de interrupci¢n
        call    dword ptr cs:rm_proc_address

		jmp     set_pm          ; SET_PM nos grabar  el estado

int_num         db ?
trash           dw ?
stack_frame     dw size txpr,?
bytes_in_stack  dw 0
rm_proc_address dd ?

; DPMI host-provided RM stack
dpmi_stack_frame     		dw DPMI_TOTAL_STACK_SIZE+DPMI_REENTRY_STACK_SIZE,?
dpmi_bytes_in_stack			dw ?
dpmi_rm_proc_address 		dd ?
dpmi_stack_post_increment	dd 0
dpmi_actual_rm_stack_frame	dd ?


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
irq16:
        lss     sp,dword ptr cs:stack_frame
        sub     sp,(3*RM_STACKSIZE/4)*4   ; por si pillamos un proc real entrando

        pushf                   ; stack frame de interrupci¢n
        call    dword ptr cs:rm_proc_address
        mov     ax,txdata
        mov     ds,ax
        jmp     set_pm_address
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
