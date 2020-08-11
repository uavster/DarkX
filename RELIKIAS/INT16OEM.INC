;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
ASSUME  DS:txdata
; rutina para realizar interrupciones 
do_int:        
;        ; si tenemos DPMI, recuperamos el estado
	mov     ax,txdata
	mov     ds,ax
;        test    system_type,2
;        jz      no_dpmi_int
;                mov     ax,txcode32
;                mov     es,ax
;                mov     di,offset idt+256*6
;                mov     al,1
;                call    dword ptr dpmi_save_restore_real
;        no_dpmi_int:

	; reajustamos la pila para modo real
        mov     ss,real_ss
        mov     esp,real_esp

	; Habilitamos la pila para que SS:ESP apunte a los valores que se 
	; hayan podido guardar antes de la int. y se los podamos pasar.
	; Igualmente, guardamos CS, EIP y EFLAGS de la pila para que la 
	; int. pueda pasarnos valores a nosotros sin miedo a 'machacarlos'
;        mov     eax,ss:[esp]
;        mov     ret_info,eax
;        mov     eax,ss:[esp+4]
;        mov     ret_info+4,eax

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
	
;        mov     ax,0b800h
;        mov     ds,ax
;        mov     al,int_num
;        mov     ah,al
;        shr     al,4
;        add     al,30h
;        mov     ds:[160*4],al
;        and     ah,0fh
;        add     ah,30h
;        mov     ds:[160*4+2],ah
;        wait3:
;        in al,60h
;        dec al
;        jnz wait3
;        jmp     a

	; provocamos la int. en modo real
		db      0CDh    ; int
	int_num db      ?
	
;        push eax
;        wait3:
;        in al,60h
;        dec al
;        jnz wait3
;        pop eax

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
