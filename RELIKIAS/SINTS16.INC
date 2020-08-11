;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; DPMI standard real interruption handler for TX32 - DOS Extender
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
ASSUME  DS:txdata
do_int:        
        mov     ax,txdata
	mov     ds,ax

	; reajustamos la pila para modo real
        mov     ss,real_ss
        mov     esp,real_esp

	; recuperamos flags de la pila
	mov     eax,pm_eax
	mov     ebx,pm_ebx
	mov     ecx,pm_ecx
	mov     edx,pm_edx
	mov     esi,pm_esi
	mov     edi,pm_edi
	mov     ebp,pm_ebp
	push    pm_eflags
	popfd
	; si CF = 1 entonces es que tenemos los reg.'s de segmento en la pila
	jnc     no_segs_restore
		add     sp,3*4
		pop     gs
		add     sp,2
		pop     fs
		add     sp,2
		pop     es
		add     sp,2
		pop     ds
		add     sp,2
		mov     cs:seg_flag,1
		jmp     go_int
	no_segs_restore:
		add     sp,3*4
		mov     cs:seg_flag,0
		clc
	go_int:
	
	; provocamos la int. en modo real
		db      0CDh    ; int
	int_num db      ?
	
	pushfd
	cmp     cs:seg_flag,1
	jnz     no_segs_save
		popfd
		push    0
		push    ds
		push    0
		push    es
		push    0
		push    fs
		push    0
		push    gs
		jmp     set_pm
	no_segs_save:
	popfd
	jmp     set_pm          ; SET_PM nos grabar  el estado
; direccionaremos esta variable con CS porque estaremos en modo REAL
seg_flag        db 0
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
