;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
real_int:	
	; Save intended RM registers in the stack, except selectors, which are meaningless in RM.
	push    dword ptr 0		; SS:SP = 0:0 tells DPMI to provide a stack
	lea		esp,[esp-2*6]	; CS:IP (ignored), GS, FS, DS, ES
	pushf
	pushad
	
	; ES:EDI -> RM registers
	mov     edi,esp
	push	es
	push    ss
	pop     es
	mov     bl,al
	mov		eax,ss:[esp+size Tdpmi_rm_regs+4]	; RM reg struc + ES
	mov		es:[edi.rm_eax],eax
	mov     ax,300h
	xor     ecx,ecx
	int     31h
	pop		es
	
	; Restore RM registers and flags (lowest byte only for "safe" flags)
	mov		al,ss:[esp+size Tpushad_regs]
	mov		ss:[esp+size Tdpmi_rm_regs+4+2*4],al	; DPMI REGS, EAX, IP, CS, ->FLAGS<-
	popad
	lea     esp,[esp+(size Tdpmi_rm_regs-size Tpushad_regs)+4]	; POP FLAGS, SS:SP, CS:IP (ignored), GS, FS, DS, ES, EAX (does not change flags)

	iretd
	
real_irq:
	push 	ds es fs gs
	pushad

	; Save intended RM registers in the stack, except selectors, which are meaningless in RM.
	push    dword ptr 0		; SS:SP = 0:0 tells DPMI to provide a stack
	lea		esp,[esp-2*6]	; CS:IP (ignored), GS, FS, DS, ES
	pushf
	pushad
	
	; ES:EDI -> RM registers
	mov     edi,esp
	push    ss
	pop     es
	mov     bl,al
	mov     ax,300h
	xor     ecx,ecx
	int     31h
	
	; Restore RM registers and flags
	lea     esp,[esp+size Tdpmi_rm_regs]	; POP RM REGS

	popad
	pop 	gs fs es ds
	pop		eax

	iretd 

real_int_old:
	push    ebx
;        mov     ebx,ss:[esp+4*4]
;        pushfd
;        and     ebx,1 SHL 9
;        or      ss:[esp],ebx        
;        popfd

	; nos guardamos los segmentos
	ASSUME  DS:txdata
	mov     bx,ds
	mov     ds,cs:D16_sel
	mov     pm_ds,bx
	mov     pm_es,es
	mov     pm_fs,fs
	mov     pm_gs,gs
	; ponemos el int_num
	mov     ds,cs:D32_sel
	mov     ebx,txcode
	lea     ebx,[ebx*8]     ; as¡, no modificamos los flags
	lea     ebx,[ebx*2]
	ASSUME  DS:txcode
	mov     [ebx+int_num],al
	pop     ebx
	pop     eax

	need_real_segs:         ; funny label (he,he...)
	pushfd
	mov     ds,cs:D16_SEL
	mov     es,cs:D32_SEL
        push    dword ptr cs:C16_SEL
	pushd   ret_real
	iretd

pm_ret_point:
	cli
	
	; ajustamos pila para modo protegido
	ASSUME  DS:txdata
	mov     ds,cs:D16_SEL
        ; primero restauramos SS:ESP de protegido para poder usar pila
        mov     esp,pm_esp
	mov     ss,pm_ss

	; restauramos todos los registros de modo real
        mov     ax,real_flags
        mov     ss:[esp+4*2],ax
        mov     eax,real_eax
	mov     ebx,real_ebx
	mov     ecx,real_ecx
	mov     edx,real_edx
	mov     esi,real_esi
	mov     edi,real_edi
	mov     ebp,real_ebp
;        push    word ptr real_flags
;        pop     word ptr ss:[esp+4*2]   ; flags devueltos por la int a protegido

        ; acabamos restaurando los registros de segmento
	mov     es,pm_es
	mov     fs,pm_fs
	mov     gs,pm_gs
	mov     ds,pm_ds        ; si restauramos antes DS, no podemos
				; direccionar RET_INFO
        iretd
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
real_irq_old:
; INPUT -> AL = interrupt number
        pushad
        push    ds es fs gs
        mov     es,cs:D32_SEL
        movzx   eax,al
        mov     ebx,es:[eax*4]  ; direcci¢n real del handle de interrupci¢n
        mov     ecx,txcode
        shl     ecx,4
        ASSUME  ES:txcode
        mov     es:[ecx+rm_proc_address],ebx
        push    dword ptr es:[ecx+saltito+1]
        mov     word ptr es:[ecx+saltito+1],offset irq16
        mov     word ptr es:[ecx+saltito+3],txcode

        mov     ds,cs:D16_SEL
        ASSUME  DS:txdata
        push    dword ptr prot_code_address
        push    dword ptr prot_code_address+4
        mov     dword ptr prot_code_address,offset back_from_irq16
        mov     pm_ss,ss
        mov     pm_esp,esp
mov al,'F'
call debug32_char
movzx eax,pm_ss
shl eax,4
add eax,pm_esp
call debug32_reg
        pushfd
        push    dword ptr cs:C16_SEL
        push    dword ptr ret_real_address
        iretd

        back_from_irq16:
        mov     ds,cs:D16_SEL
        mov     ss,pm_ss
        mov     esp,pm_esp
mov al,'G'
call debug32_char
movzx eax,pm_ss
shl eax,4
add eax,pm_esp
call debug32_reg
        pop     dword ptr prot_code_address+4
        pop     dword ptr prot_code_address
        mov     ds,cs:D32_SEL
        mov     ecx,txcode
        shl     ecx,4
        ASSUME  DS:txcode
        pop     dword ptr [ecx+saltito+1]
        ASSUME  DS:txdata
        pop     gs fs es ds
        popad
        pop     eax
        iretd
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

