;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
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