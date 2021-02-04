;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
; Standard DPMI 0.9 Real Mode Call and Interrupt execution with segment pass
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�

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


; DPMI host-provided RM stack
dpmi_stack_frame     		dw DPMI_TOTAL_STACK_SIZE+DPMI_REENTRY_STACK_SIZE,?
dpmi_bytes_in_stack			dw ?
dpmi_rm_proc_address 		dd ?
dpmi_stack_post_increment	dd 0
dpmi_actual_rm_stack_frame	dd ?