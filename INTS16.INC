;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Standard DPMI 0.9 Real Mode Call and Interrupt execution with segment pass
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

dpmi_rm_retf:
		; This is called after a PM-to-RM mode change. Therefore, interrupts are disabled,
		; which is what we need as we mess with the stack.
        ASSUME  SS:txdata
        mov     ax,txdata
        mov     ss,ax
		
		; Get call address
        mov     eax,dword ptr ss:[dpmi_rm_call_struc.rm_ip]
        mov		dword ptr cs:dpmi_rm_proc_address,eax

		; Set up all registers from passed DPMI structure
        mov     sp,offset dpmi_rm_call_struc
        popad
        pop     cs:dpmi_trash        ; Do not pop passed flags yet, since we don't have a stack (interrupts could be enabled)
        pop     es ds fs gs
        lss     sp,dword ptr ss:[dpmi_rm_call_struc.rm_sp]

		; We have now a stack. Restoring the passed flags is now OK, even if interrupts are enabled.
		push	cs:dpmi_trash
		popf
		
		call    dword ptr cs:dpmi_rm_proc_address
		
		; The target RM procedure must leave the stack as it found it.
		
		; Disable interrupts before messing with the stack
		pushf
		pop		cs:dpmi_trash
		cli
		
		; Copy registers back to DPMI structure
		lea		sp,ss:[dpmi_rm_call_struc.rm_ip]
		push	gs fs ds es
		push	cs:dpmi_trash
		pushad

		; Restore stack in case it's needed for the mode switch
		lss		sp,dword ptr ss:[dpmi_rm_call_struc.rm_sp]
		
		; Back to PM
		jmp     set_pm_dpmi


dpmi_rm_iret:
		; This is called after a PM-to-RM mode change. Therefore, interrupts are disabled,
		; which is what we need as we mess with the stack.
        ASSUME  SS:txdata
        mov     ax,txdata
        mov     ss,ax
 		
		; Get call address
        mov		eax,dword ptr ss:[dpmi_rm_call_struc.rm_ip]
        mov     dword ptr cs:dpmi_rm_proc_address,eax

		; Set up all registers from passed DPMI structure
        mov     sp,offset dpmi_rm_call_struc
        popad
        pop     cs:dpmi_trash        ; Do not pop passed flags yet, since interrupts cannot be enabled
        pop     es ds fs gs
        lss     sp,dword ptr ss:[dpmi_rm_call_struc.rm_sp]

		; We have now a stack. Restoring the passed flags is now OK, even if interrupts are enabled.
		push	cs:dpmi_trash
;		and		byte ptr ss:[esp],0feh
		popf

		pushf	; Interrupt stack frame
		call    dword ptr cs:dpmi_rm_proc_address	

		; The target RM procedure must leave the stack as it found it.
		
		; Disable interrupts before messing with the stack (in case the target procedure enabled them in the stack before IRET)
		pushf
		pop		cs:dpmi_trash
		cli
		
		; Copy registers back to DPMI structure
		push	txdata
		pop 	ss
		lea		sp,ss:[dpmi_rm_call_struc.rm_ip]
		push	gs fs ds es
		push	cs:dpmi_trash
		pushad

		; Restore stack in case it's needed for the mode switch
		lss		sp,dword ptr ss:[dpmi_rm_call_struc.rm_sp]

push eax
mov al,'$'
call debug16_char
pop eax		
call debug16_reg

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

dpmi_trash           		dw ?
dpmi_stack_frame     		dw DPMI_TOTAL_STACK_SIZE+DPMI_REENTRY_STACK_SIZE,?
dpmi_bytes_in_stack			dw ?
dpmi_rm_proc_address 		dd ?
dpmi_stack_post_increment	dd 0


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
