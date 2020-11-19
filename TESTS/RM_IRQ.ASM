.386p
.model flat
.stack 400h
.code
Trm_reg         STRUC
        rm_edi          dd ?
        rm_esi          dd ?
        rm_ebp          dd ?
                        dd ?
        rm_ebx          dd ?
        rm_edx          dd ?
        rm_ecx          dd ?
        rm_eax          dd ?
        rm_flags        dw ?
        rm_es           dw ?
        rm_ds           dw ?
        rm_fs           dw ?
        rm_gs           dw ?
        rm_ip           dw ?
        rm_cs           dw ?
        rm_sp           dw ?
        rm_ss           dw ?
ENDS

ErrorExit       MACRO error_msg
LOCAL   skip_msg,msg
        jnc     skip_msg
        push    ss
        pop     ds
        mov     ah,9
        mov     edx,offset msg
        int     21h
        jmp     exit_from_here
msg     db error_msg,"$"
skip_msg:
ENDM

;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
; Changes the frequency of the system timer
; INPUT -> EAX = int frequency in Hz (max. 1193182 Hz)
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
set_timer_freq  proc
        ; 1193182/frequency is given to the controller
        mov     ebx,eax
        mov     eax,1193182     ; magic constant ;) (max. frequency)
        xor     edx,edx
        div     ebx
        cmp     eax,65536       ; max. port value = 65536 (=0)
        jb      no_fixup
                xor     eax,eax
        no_fixup:
        mov     ebx,eax
        mov     al,36h
        out     43h,al
        mov     al,bl
        out     40h,al
        mov     al,bh
        out     40h,al        
        ret
set_timer_freq  endp

;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
; Locks a memory region
;
; INPUT -> DS:EBX -> memory region to lock
;          ECX = size of region to lock
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
LockMem proc
        push    ecx
        mov     ecx,ebx
        shr     ebx,16
        pop     edi
        mov     esi,edi
        shr     esi,16
        mov     ax,600h
        int     31h
        ret
LockMem endp

;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
; Unlocks a memory region
;
; INPUT -> DS:EBX -> memory region to unlock
;          ECX = size of region to unlock
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
UnlockMem       proc
        push    ecx
        mov     ecx,ebx
        shr     ebx,16
        pop     edi
        mov     esi,edi
        shr     esi,16
        mov     ax,601h
        int     31h
        ret
UnlockMem       endp

start:  jmp short main
        db "WATCOM"
main:
        mov     segmento,ds
        push    ds
        pop     es

		; Save old IRQ handler
        mov     ax,200h
        mov     bl,8
        int     31h
        mov     old_handler,dx
		mov		old_handler+2,cx

		; Lock memory touched by the IRQ handler
		mov		ebx,offset timer_isr
		mov		ecx,offset end_of_lock_area-offset timer_isr
		call	LockMem
		
		; Get a real mode callback to our protected mode routine
        mov     edi,offset rmcallstruc
        push    cs
        pop     ds
        mov     ax,303h
        mov     esi,offset timer_isr
        int     31h
        ErrorExit "Unable to get real mode callback address"
		mov		rm_callback_seg,cx
		mov		rm_callback_off,dx
		
		; Point timer IRQ0 to the real mode callback
        mov     ax,201h
        mov     bl,8
        int     31h
        ErrorExit "Unable to set real mode interrupt"

		; Set timer frequency
		mov		eax,18
		call	set_timer_freq

        ; Wait for key in real mode.
		; With a protected mode interrupt, our timer routine would never be called.
		; The callback that IRQ0 points to calls our protected mode timer routine from real mode.
		mov     ah,8
		int     21h

        exit_from_here:
		; Restore interrupt handler
        mov     ax,201h
		mov		cx,old_handler+2
		mov		dx,old_handler
        mov     bl,8
        int     31h
		
		; Free callback
		mov		ax,304h
		mov		cx,rm_callback_seg
		mov		dx,rm_callback_off
		int		31h
		
		; Unlock memory
		mov		ebx,offset timer_isr
		mov		ecx,offset end_of_lock_area-offset timer_isr
		call	UnlockMem
		
		; Exit
        mov     ah,4Ch
        int     21h

timer_isr:
        mov     ds,cs:segmento
        mov     al,hihi
        mov     ah,1eh
        mov     ds:[0b8000h+4*160],ax
        inc     al
        cmp     al,3Ah
        jnz     nada
                mov     al,30h
        nada:
        mov     hihi,al

		mov     al,20h
        out     20h,al

        cld
        lodsw
        mov     es:[edi.rm_ip],ax
        lodsw
        mov     es:[edi.rm_cs],ax
        lodsw
        mov     es:[edi.rm_flags],ax
        add     es:[edi.rm_sp],2*3
		
        iretd
segmento        dw ?
old_handler		dw ?
				dw ?
rm_callback_off	dw ?
rm_callback_seg	dw ?
hihi            db 30h
end_of_lock_area:

.data
rmcallstruc     dd 100 dup(?)
end start
