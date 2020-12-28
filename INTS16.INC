;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Standard DPMI 0.9 Real Mode Call and Interrupt execution with segment pass
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

dpmi_rm_retf:
        ASSUME  SS:txdata
        mov     ax,txdata
        mov     ss,ax
        lea     sp,ss:[dpmi_rm_call_struc.rm_ip]
        pop     dword ptr cs:dpmi_rm_proc_address

        mov     sp,offset dpmi_rm_call_struc
        popad
        pop     cs:dpmi_trash        ; dpmi_trash = FLAGS para no cambiarlos con el CMP
        pop     es
        pop     ds
        pop     fs
        pop     gs
        lss     sp,ss:[esp+2*2]

		call    dword ptr cs:dpmi_rm_proc_address
		jmp     set_pm_dpmi

dpmi_rm_iret:
        cli

        ASSUME  SS:txdata
        mov     ax,txdata
        mov     ss,ax
        lea     sp,ss:[dpmi_rm_call_struc.rm_ip]
        pop     dword ptr cs:dpmi_rm_proc_address

        mov     sp,offset dpmi_rm_call_struc
        popad
        pop     cs:dpmi_trash        ; dpmi_trash = FLAGS para no cambiarlos con el CMP
        pop     es
        pop     ds
        pop     fs
        pop     gs
        lss     sp,ss:[esp+2*2]

pushad
push ds
mov     ax,txdata
mov     ds,ax
ASSUME DS:txdata
cmp word ptr ds:[dpmi_rm_call_struc.rm_ebp],302h
jnz skip_debug
mov al,'|'
call debug16_char
mov al,'Q'
call debug16_char
;mov eax,esp
push ds
mov ax,txcode32
mov ds,ax
ASSUME DS:TXCODE32
mov eax,ds:tmp_esi
mov ebx,eax
shr ebx,4
mov ds,bx
and eax,0fh
mov si,ax
mov ax,ds:[si+2*2]
pop ds
ASSUME DS:TXDATA
call debug16_reg
mov al,':'
call debug16_char
xor eax,eax
mov ax,ss
shl eax,4
movzx esp,sp
add eax,esp
call debug16_reg
skip_debug:
pop ds
popad

		push    cs:dpmi_trash        ; para formar un stack frame de interrupci¢n
		call    dword ptr cs:dpmi_rm_proc_address	
		jmp     set_pm_dpmi

;int_or_call     dw ?
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

push eax		
mov al,'|'
call debug16_char
mov al,'I'
call debug16_char
xor eax,eax
mov ax,ss
shl eax,4
movzx esp,sp
add eax,esp
call debug16_reg
pop eax

        pushf           ; para formar un stack frame de interrupci¢n
        call    dword ptr cs:rm_proc_address

	jmp     set_pm          ; SET_PM nos grabar  el estado

int_num         db ?
trash           dw ?
stack_frame     dw size txpr,?
bytes_in_stack  dw 0
rm_proc_address dd ?

dpmi_trash           dw ?
dpmi_stack_frame     dw DPMI_TOTAL_STACK_SIZE+DPMI_REENTRY_STACK_SIZE,?
dpmi_bytes_in_stack		dw ?
dpmi_rm_proc_address dd ?
dpmi_stack_post_increment dw 0
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
irq16:
        lss     sp,dword ptr cs:stack_frame
        sub     sp,(RM_STACKSIZE/2)*4   ; por si pillamos un proc real entrando

push eax		
mov al,'|'
call debug16_char
mov al,'i'
call debug16_char
xor eax,eax
mov ax,ss
shl eax,4
movzx esp,sp
add eax,esp
call debug16_reg
pop eax

        pushf                   ; stack frame de interrupci¢n
        call    dword ptr cs:rm_proc_address
        mov     ax,txdata
        mov     ds,ax
        jmp     set_pm_address
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
