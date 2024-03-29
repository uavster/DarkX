;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Standard DPMI 0.9 INT 31h support routines
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
tx_int31h:
        ; TX32 OEM functions
        cmp     ax,3200h        ; get system information
        jz      get_sys_info
        cmp     ax,3201h        ; get program information
        jz      get_program_info

        ; for DOS32 compatibility
        cmp     ax,0EE00h
        jz      dos32_info      ; get DOS32 version 
        cmp     ax,0EE02h
        jz      dos32_sys_info  ; get DOS32 address information

        push    ds
        mov     ds,cs:D16_SEL
        ASSUME  DS:txdata
        test    system_type,2
        pop     ds
        jnz     dpmi_handles_standard_funcs

        cmp     ax,500h
        jz      ext_mem_info    ; get free memory info
        cmp     ax,501h
        jz      ext_mem_alloc   ; allocate memory block
        cmp     ax,502h
        jz      ext_mem_free    ; free memory block
        cmp     ax,503h
        jz      ext_mem_resize  ; resize memory block

        cmp     ax,100h
        jz      dos_mem_alloc   ; allocate DOS memory
        cmp     ax,101h
        jz      dos_mem_free    ; free DOS memory block
        cmp     ax,102h
        jz      dos_mem_resize  ; resize DOS memory block

        cmp     ax,200h         ; get real mode interrupt vector
        jz      get_rm_int_vec
        cmp     ax,201h
        jz      set_rm_int_vec  ; set real mode interrupt vector

        cmp     ax,300h
        jz      make_rm_int     ; simulate RM interrupt
        cmp     ax,301h
        jz      make_rm_int     ; RM call with FAR RET frame
        cmp     ax,302h
        jz      make_rm_int     ; RM call with IRET frame

        cmp     ax,303h
        jz      callback_alloc  ; allocate real mode call-back address
        cmp     ax,304h
        jz      callback_free   ; free real mode call-back address

        cmp     ax,204h
        jz      get_int_vector
        cmp     ax,205h
        jz      set_int_vector

        or      ax,ax
        jz      alloc_desc      ; allocate LDT descriptors
        cmp     ax,1
        jz      free_desc       ; free LDT descriptor
        cmp     ax,3
        jz      next_desc       ; get next selector increment value
        cmp     ax,0Ch
        jz      set_desc        ; set descriptor
        cmp     ax,6
        jz      get_seg_base    ; get segment base    

        cmp     ax,0800h
        jz      map_memory
		
		; Ignore locking/unlocking services, as we do not support virtual memory
		cmp		ax,600h			; lock linear region
		jz		return_success
		cmp		ax,601h			; unlock linear region
		jz		return_success

        ; si no es una funci¢n soportada, salimos con CF = 1
        or      byte ptr ss:[esp+4*2],1
        iretd

        dpmi_handles_standard_funcs:
        pushfd
        call    fword ptr cs:[idt+31h*6]
        pushfd
        pop     dword ptr ss:[esp+4*2]  ; ponemos los flags devueltos
        iretd

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Real Mode Services for Interrupt and Far Call execution
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
DPMI_TOTAL_STACK_SIZE EQU 4*RM_STACKSIZE
DPMI_REENTRY_STACK_SIZE EQU DPMI_TOTAL_STACK_SIZE/DPMI_MAX_REENTRIES

make_rm_int:
        pushad
		push	ds es fs gs
		sub		esp,4+4
		mov		ebp,esp		; Space for dpmi_stack_post_increment and dpmi_actual_stack_frame
		mov		ds,cs:D32_SEL
        push    es edi

		; Get RM proc address from int vector table if service is 300h
		cmp		ax,300h
		jnz		no_int_service
				movzx	ebx,bl
				mov		ebx,ds:[ebx*4]
				mov		dword ptr es:[edi.rm_ip],ebx
		no_int_service:

        ; Assign a stack to RM, if not provided
		mov		ebx,txcode
		shl		ebx,4
		ASSUME	DS:txcode
		pushfd
		cli
		mov		edx,ds:[ebx+dpmi_stack_post_increment]
		mov		ss:[ebp],edx	; Push dpmi_stack_post_increment
		mov		ds:[ebx+dpmi_stack_post_increment],0
        mov     edx,dword ptr es:[edi.rm_sp]
        or      edx,edx
        jnz     stack_provided
				; Provide a real-mode stack for the call
				mov		ds:[ebx+dpmi_stack_post_increment],DPMI_REENTRY_STACK_SIZE
				sub     word ptr ds:[ebx+dpmi_stack_frame],DPMI_REENTRY_STACK_SIZE
				mov     word ptr ds:[ebx+dpmi_stack_frame+2],txpila_real
				mov		edx,dword ptr ds:[ebx+dpmi_stack_frame]
        stack_provided:
		sub		dx,size Tdpmi_rm_regs	; Reserve space for client-passed RM registers
		mov		dword ptr es:[edi.rm_sp],edx
		xchg	ds:[ebx+dpmi_actual_rm_stack_frame],edx
		mov		ss:[ebp+4],edx			; Push previous RM stack frame to current PM stack
		popfd
		
        ; Copy CX words from PM stack to RM stack
		lea		esi,ss:[esp+size Tpushad_regs+(4+2+3)*4]	; Stack before int 31h, i.e. pop edi es gs fs es ds popad pop eip cs popfd
		push	esi ecx
        or      cx,cx
        jz      no_stack_parameters
				push    ss
				pop     ds
				; Simulate pushing all stack parameters (subtract twice as ECX is number of parameter WORDS)
				mov		edx,ds:[ebx+dpmi_actual_rm_stack_frame]
				sub		dx,cx
				sub		dx,cx
				mov		dword ptr es:[edi.rm_sp],edx
				mov     es,cs:D32_SEL
				ASSUME  ES:txcode
				; calculamos el offset lineal de la pila real
				xor     edi,edi
				mov     di,dx
				and     edx,0FFFF0000h
				shr     edx,16-4
				add     edi,edx
				; lo copiamos todo
				cld
				rep     movsw
				ASSUME  ES:txdata
        no_stack_parameters:

        ; Copy DPMI structure to RM stack (below 1 Mb) for access from RM
        pop     ecx edi esi ds	; DS:ESI -> client-passed RM registers
        push    ds esi edi ecx
		; Push RM register to RM stack frame
		mov		edi,dword ptr ds:[esi.rm_sp]
		mov		es,cs:D32_SEL
		movzx	edx,di
		and		edi,0ffff0000h
		shr		edi,16-4
		add		edi,edx		; ES:EDI -> RM stack, DS:SI -> client-passed RM registers
        mov     ecx,(size Tdpmi_rm_regs)/2
        cld
        rep     movsw

		; Call real mode routine.
		; Inside this routine, and in RM, the DPMI input structure is pointed by txcode:dpmi_actual_rm_stack_frame, ready to pop.
		; The RM call will be called with the following stack:
		; RM_REGS.RM_SS:RM_SP ->			Client-passed PM stack words
		; dpmi_actual_rm_stack_frame ->		RM registers
		; 						 			RM stack
		mov cx,txcode
		cmp	ax,301h
		jnz call_with_iret
				mov dx,offset dpmi_rm_retf
				jmp call_real_mode
		call_with_iret:
				mov dx,offset dpmi_rm_iret
		call_real_mode:
		call dpmi_call_rm

		; Copy stack parameters in RM stack back to PM stack
		pop		ecx		; ECX = Number of stack parameter words
		pop		esi		; SS:ESI -> Stack before int 31h
		pop 	edi es	; ES:EDI -> Passed RM struct with registers
		push	es edi
		mov		ebx,dword ptr es:[edi.rm_sp]
        push	ss
		pop		es
		mov		edi,esi	; ES:EDI -> Stack before int 31h
        or      cx,cx
        jz      no_return_vals
				ASSUME  DS:txcode
				mov     ds,cs:D32_SEL
				movzx	esi,bx
				and     ebx,0FFFF0000h
				shr     ebx,16-4
				add     esi,ebx
				cld
				rep     movsw
				ASSUME  DS:txdata
        no_return_vals:
		
        ; copiamos todos los valores obtenidos en la estructura
        pop     edi es
        mov     ds,cs:D32_SEL
        ASSUME  DS:txcode
		mov		ebx,txcode
		shl		ebx,4
        mov     esi,ds:[ebx+dpmi_actual_rm_stack_frame]
		movzx	ebx,si
		and		esi,0ffff0000h
		shr		esi,16-4
		add		esi,ebx
        mov     ecx,(size Tdpmi_rm_regs)/2
        cld
        rep     movsw
		
		; Free provided stack 
		mov		ds,cs:D32_SEL
		mov		ebx,txcode
		shl		ebx,4
		ASSUME	DS:txcode
		cli
		mov		edx,ds:[ebx+dpmi_stack_post_increment]
		add     word ptr ds:[ebx+dpmi_stack_frame],dx
		
		; Restore post-increment, as this call is reentrant
		pop	ds:[ebx+dpmi_stack_post_increment]
		pop ds:[ebx+dpmi_actual_rm_stack_frame]
        ; restauramos los registros
		pop gs fs es ds
        popad
		return_success:
		and byte ptr ss:[esp+2*4],0feh	; Clear carry flag
        iretd

dpmi_call_rm proc
; INPUT: 	CX:DX -> Real mode entry point to call. Must end with "jmp set_pm_dpmi".

	; Disable interrupts since we're modifying critical state.
	; Flags in real mode will be set according to the DPMI structure passed by the caller.
	pushfd
	cli

	; Save current RM entry point and fix with new address
	mov     ds,cs:D32_SEL
	mov     edi,txcode
	shl     edi,4
	ASSUME  DS:txcode
	push	dword ptr ds:[edi+saltito+1]
	mov     word ptr ds:[edi+saltito+1],dx
	mov		word ptr ds:[edi+saltito+3],cx
	
	; Save current PM return point and fix with new address
	ASSUME  DS:txdata
	mov     ds,cs:D16_SEL
	push	dword ptr ds:prot_code_address
	push	dword ptr ds:prot_code_address+4
	mov     dword ptr ds:prot_code_address,offset back_from_rm_call
	mov		word ptr ds:prot_code_address+4,cs

	; Will restore selectors before leaving to ensure that none is
	; left with an RM segment that is invalid in PM
	push ds es fs gs
	
	; Save PM stack
	mov eax,ds:dpmi_stack_info_offset
	add eax,offset dpmi_stack_info
	add ds:dpmi_stack_info_offset,size Tstack_info
	mov ds:[eax.stack_esp],esp
	mov ds:[eax.stack_ss],ss

	; Far jump to RM entry point
	mov	es,cs:D32_SEL
    push dword ptr cs:C16_SEL
    push dword ptr ret_real_address
	retf
	back_from_rm_call:
	
	; Recover PM stack
	mov     ds,cs:D16_SEL
	ASSUME  DS:txdata
	sub ds:dpmi_stack_info_offset,size Tstack_info
	mov	eax,ds:dpmi_stack_info_offset
	lss esp,ds:[eax+dpmi_stack_info]

	; Restore selectors before leaving to ensure that none is
	; left with an RM segment that is invalid in PM
	pop gs fs es ds
	
	; Restore PM return point
	pop		dword ptr ds:prot_code_address+4
	pop		dword ptr ds:prot_code_address
	
	; Restore RM entry point
	mov     ds,cs:D32_SEL
	mov     eax,txcode
	shl     eax,4
	ASSUME  DS:txcode
	pop		dword ptr ds:[eax+saltito+1]
	
	popfd
	ret
endp

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; DOS Memory Management Services
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
ASSUME  DS:txcode32
dos_mem_alloc:
        or      byte ptr ss:[esp+4*2],1         ; CF = 1 por si hay error
        push    ds ebx ecx
        mov     ds,cs:D32_SEL
        mov     eax,txcode32
        shl     eax,4
        cmp     byte ptr [eax+dos_pointers],0
        jz      no_dos_heap_error
        add     eax,offset dos_heap_info
        movzx   ecx,bx
        shl     ecx,4
        push    eax
        call    malloc
        pop     eax
        or      ebx,ebx
        jz      dos_alloc_error
        mov     eax,ebx
        shr     eax,4   ; estar  alineada a p rrafo pq lo est  toda la heap
        mov     edx,eax ; selector = segmento para handling posterior
        pop     ecx ebx ds
        and     byte ptr ss:[esp+4*2],0FEh ; CF = 0 pq no hay error
        iretd
dos_alloc_error:
        ; buscamos el bloque m s grande memoria para devolverlo en BX
        call    getmeminfo
        mov     ebx,eax
        shr     ebx,4
        mov     ax,8                    ; error code = not enough memory
        pop     ecx
        add     esp,4
        pop     ds
        or      byte ptr ss:[esp+4*2],1 ; CF = 1 por si error
        iretd
no_dos_heap_error:
        pop     ecx ebx ds
        mov     ax,8                    ; error 8 = not enough memory
        xor     ebx,ebx                 ; 0 paragraphs available
        or      byte ptr ss:[esp+4*2],1 ; CF = 1 por si error
        iretd
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
dos_mem_free:
        push    ds eax ebx ecx
        mov     ds,cs:D32_SEL
        mov     ebx,txcode32
        shl     ebx,4
        cmp     byte ptr [ebx+dos_pointers],0
        jz      no_dos_heap_error1
        movzx   ebx,dx
        shl     ebx,4
        call    free
        pop     ecx ebx eax ds
        and     byte ptr ss:[esp+4*2],0FEh
        iretd
no_dos_heap_error1:
        pop     ecx ebx eax ds
        mov     ax,9                    ; error 9 = invalid block segment
        or      byte ptr ss:[esp+4*2],1
        iretd
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
dos_mem_resize:
        push    ds eax ecx edx
        mov     ds,cs:D32_SEL
        mov     eax,txcode32
        shl     eax,4
        cmp     byte ptr [eax+dos_pointers],0
        jz      no_dos_heap_error2
        add     eax,offset dos_heap_info
        movzx   ecx,bx
        shl     ecx,4
        movzx   ebx,dx
        shl     ebx,4
        push    eax esi edi 
        call    realloc
        pop     edi esi eax edx
        or      ebx,ebx
        jz      dos_alloc_error
        shr     ebx,4
        mov     edx,ebx
        pop     ecx eax ds
        and     byte ptr ss:[esp+4*2],0FEh
        iretd
no_dos_heap_error2:
        pop     edx ecx eax ds
        mov     ax,9                    ; error 9 = invalid block segment
        or      byte ptr ss:[esp+4*2],1
        iretd

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Extended Memory Management Services
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
ext_mem_info:
        or      byte ptr ss:[esp+4*2],1
        push    ds eax ebx ecx edi
        xor     eax,eax
        dec     eax
        mov     ecx,30h/4
        cld
        rep     stosd
        pop     edi
        inc     eax
        mov     es:[edi],eax    ; 0 bytes si no hay heap
        mov     ds,cs:D32_SEL
        mov     eax,txcode32
        shl     eax,4
        cmp     byte ptr [eax+extended_pointers],0
        jz      no_ext_heap_error
        add     eax,offset extended_heap_info
        call    getmeminfo
        mov     es:[edi],eax
        and     byte ptr ss:[esp+4*6],0FEh
        no_ext_heap_error:
        pop     ecx ebx eax ds
        iretd
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
ext_mem_alloc:
        push    eax ebx ecx ds
        mov     ds,cs:D32_SEL
        mov     eax,txcode32
        shl     eax,4
        cmp     byte ptr [eax+extended_pointers],0
        jz      alloc_error     ; salta si no hay heap
        add     eax,offset extended_heap_info
        shl     ebx,16
        and     ecx,0FFFFh
        or      ecx,ebx
        call    malloc
        or      ebx,ebx
        jz      alloc_error
        pop     ds
        mov     ecx,ebx
        shr     ebx,16
        mov     esi,ebx                         ; handle = linear address
        mov     edi,ecx
        add     esp,4*2
        pop     eax
        and     byte ptr ss:[esp+4*2],0FEh
        iretd
alloc_error:
        pop     ds ecx ebx eax
        or      byte ptr ss:[esp+2*4],1         ; CF = 1 si por si hay error
        iretd
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
ext_mem_free:
        or      byte ptr ss:[esp+4*2],1
        push    ds eax ebx ecx
        mov     ds,cs:D32_SEL
        mov     ebx,txcode32
        shl     ebx,4
        cmp     byte ptr [ebx+extended_pointers],0
        jz      ext_free_error
        mov     ebx,esi
        shl     ebx,16
        mov     bx,di
        call    free
        and     byte ptr ss:[esp+4*6],0FEh
        ext_free_error:
        pop     ecx ebx eax ds
        iretd
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
ext_mem_resize:
        push    eax edx ebx ecx esi edi ds
        mov     ds,cs:D32_SEL
        mov     eax,txcode32
        shl     eax,4
        cmp     byte ptr [eax+extended_pointers],0
        jz      realloc_error
        add     eax,offset extended_heap_info
        and     ecx,0FFFFh
        shl     ebx,16
        or      ecx,ebx
        mov     ebx,esi
        shl     ebx,16
        mov     bx,di
        call    realloc
        or      ebx,ebx
        jz      realloc_error
        pop     ds
        mov     edi,ebx
        mov     esi,ebx
        shr     esi,16
        mov     ebx,esi
        mov     ecx,edi
        add     esp,4*4
        pop     edx eax
        and     byte ptr ss:[esp+4*2],0FEh
        iretd
realloc_error:
        pop     ds edi esi ecx ebx edx eax
        or      byte ptr ss:[esp+4*2],1
        iretd

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; LDT Access Services
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
ASSUME  DS:txdata
alloc_desc:
        push    eax ds ebx
        mov     ds,cs:D16_SEL
        xor     eax,eax
        mov     ebx,FREE_DESCS
        movzx   ecx,cx

        search_free_descs:
                or      ebx,ebx
                jz      not_enough_descs
                cmp     byte ptr [eax+free_descs_handles],0
                jz      found_one_slot
                inc     eax
        dec     ebx
        jnz     search_free_descs
        jmp     not_enough_descs
        found_one_slot:
        push    ecx eax
        find_the_rest:
                cmp     byte ptr [eax+free_descs_handles],0
                jnz     couldnt_find_em
                inc     eax
                dec     ebx
        loop    find_the_rest
        jmp     found_descs
        couldnt_find_em:
        add     esp,4
        pop     ecx
        jmp     search_free_descs

        found_descs:
        pop     eax ecx ebx
        push    eax ecx
        alloc_em:
                inc     byte ptr [eax+free_descs_handles]
                mov     dword ptr [eax*8+free_descs_start],0
                mov     dword ptr [eax*8+free_descs_start+4],0C09300h
                inc     eax
        loop    alloc_em
        pop     ecx eax
        add     eax,NUM_DESCS
        shl     eax,3
        pop     ds
        add     esp,4
        and     byte ptr ss:[esp+4*2],0FEh
        iretd

        not_enough_descs:
        pop     ebx ds eax
        or      byte ptr ss:[esp+4*2],1
        iretd
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
free_desc:
        push    ds ebx
        mov     ds,cs:D16_SEL
        movzx   ebx,bx
        shr     bx,3
        or      byte ptr ss:[esp+4*4],1
        cmp     bl,NUM_DESCS
        jb      free_desc_error
        cmp     bl,NUM_DESCS+FREE_DESCS
        jae     free_desc_error
        mov     byte ptr [ebx+free_descs_handles-NUM_DESCS],0
        and     byte ptr ss:[esp+4*4],0FEh
        free_desc_error:
        pop     ebx ds
        iretd
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
next_desc:
        mov     ax,8
        and     byte ptr ss:[esp+4*2],0FEh
        iretd
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
set_desc:
        push    ds ebx
        mov     ds,cs:D16_SEL
        movzx   ebx,bx
        shr     bx,3
        or      byte ptr ss:[esp+4*4],1
        cmp     ebx,NUM_DESCS
        jb      set_desc_error
        cmp     ebx,NUM_DESCS+FREE_DESCS
        jae     set_desc_error
        cmp     byte ptr [ebx+free_descs_handles-NUM_DESCS],0
        jz      set_desc_error
        push    dword ptr es:[edi]
        pop     dword ptr [ebx*8+gdt]
        push    dword ptr es:[edi+4]
        pop     dword ptr [ebx*8+gdt+4]
        and     byte ptr ss:[esp+4*4],0FEh
        set_desc_error:
        pop     ebx ds
        iretd
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
get_seg_base:
        or      byte ptr ss:[esp+4*2],1
        push    ebx ds
        mov     ds,cs:D16_SEL
        movzx   ebx,bx
        shr     bx,3
        cmp     bx,NUM_DESCS+FREE_DESCS
        jae     get_seg_base_error
        cmp     bx,NUM_DESCS
        jb      get_this_base
        cmp     byte ptr [ebx+free_descs_handles-NUM_DESCS],0
        jz      get_seg_base_error
        get_this_base:
        mov     dx,word ptr [ebx*8+gdt+2]
        mov     cl,byte ptr [ebx*8+gdt+4]
        mov     ch,byte ptr [ebx*8+gdt+7]
        and     byte ptr ss:[esp+4*4],0FEh
        get_seg_base_error:
        pop     ds ebx
        iretd

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Interrupt Vector Services
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
ASSUME  DS:txcode32
get_int_vector:
        push    ds eax ebx
        mov     ds,cs:D32_SEL
        mov     eax,txcode32
        shl     eax,4
        movzx   ebx,bl
        cmp     bl,8
        jb      no_irq_fixup1
        cmp     bl,15
        ja      no_irq_fixup1
                add     bl,NEW_IRQ0_IRQ7_BASE-8        ; irq's redireccionadas
        no_irq_fixup1:
        mov     cx,word ptr [ebx*8+eax+idt+2]
        mov     dx,word ptr [ebx*8+eax+idt+6]
        shl     edx,16
        mov     dx,word ptr [ebx*8+eax+idt]
        pop     ebx eax ds
        and     byte ptr ss:[esp+4*2],0FEh
        iretd
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
set_int_vector:
        push    ds eax ebx
        cli
        mov     ds,cs:D32_SEL
        mov     eax,txcode32
        shl     eax,4
        movzx   ebx,bl
        cmp     bl,8
        jb      no_irq_fixup2
        cmp     bl,15
        ja      no_irq_fixup2
                add     bl,NEW_IRQ0_IRQ7_BASE-8           
        no_irq_fixup2:
        mov     word ptr [ebx*8+eax+idt+2],cx
        mov     word ptr [ebx*8+eax+idt],dx
        shr     edx,16
        mov     word ptr [ebx*8+eax+idt+6],dx
        sti
        pop     ebx eax ds
        and     byte ptr ss:[esp+4*2],0FEh
        iretd
ASSUME  DS:txdata

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Memory Mapping Services
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
map_memory:
        and     byte ptr ss:[esp+4*2],0FEh
        push    ds
        mov     ds,cs:D16_SEL
        test    system_type,1
        jz      no_mapping_needed
                cli
                ; s¢lo hacemos algo si tenemos VCPI
                pushad
                push    es
                push    bx
                push    cx
                pop     ebx             ; EBX = linear address of region
                and     bx,0F000h       ; 4Kb alignment
                cmp     ebx,100000h     ; si est  en el primer Mb
                jbe     no_pageable     ; no se puede paginar
                push    si
                push    di
                pop     ecx             ; ECX = size of region in bytes
                or      ecx,ecx
                jz      no_pageable
                add     cx,0FFFh
                and     cx,0F000h       ; 4Kb multiple
                shr     ecx,10          ; size/1024

                ; alojamos 1Kb por cada 1Mb que queramos paginar
                push    ebx ecx
                mov     ax,100h
                mov     ebx,ecx
                shr     ebx,4           ; /16 -> p rrafos
                add     bx,4096/16      ; para alinear a 4Kb
                int     31h
                pop     ecx ebx
                jc      no_pageable
                add     ax,0FFh
                and     ax,0FF00h       ; alineamos
                movzx   eax,ax
                shl     eax,4   ; EAX = linear address of first entry

                ; buscamos p ginas con el present bit = 0
                mov     es,cs:D32_SEL
                mov     edi,page_dir_base
                mov     edx,ecx
                add     dx,0FFFh
                and     dx,0F000h
                shr     edx,12          ; EDX = page dir. entries needed
                mov     ebp,page_dir_entries
                search_nopresent_entries:
                        push    edx
                        test    byte ptr es:[edi],1
                        jnz     present_entry
                        look_for_enough_space:
                                test    byte ptr es:[edi],1
                                jnz     present_entry
                                add     edi,4
                        dec     edx
                        jnz     look_for_enough_space
                        jmp     found_space_for_paging
                        present_entry:
                        add     edi,4
                        pop     edx
                dec     ebp
                jnz     search_nopresent_entries
                jmp     no_pageable

                found_space_for_paging:
                pop     edx
                shl     edx,2
                sub     edi,edx          ; EDI -> group of free entries

                ; entradas del directorio de p gs. = tablas de p gs.
                mov     edx,ecx
                add     dx,0FFFh         ; round up size to 4Mb
                and     dx,0F000h
                shr     edx,12
                push    eax
                or      al,7
                push    edi
                cld
                set_dir_entries:
                        stosd
                        add     eax,1000h
                dec     edx
                jnz     set_dir_entries
                pop     edi eax

                shr     ecx,2           ; size/4Kb
                or      bl,67h          ; dirty,accessed,user,writeable,present
                map_the_region:
                        mov     es:[eax],ebx
                        add     ebx,1000h
                        add     eax,4
                loop    map_the_region
                sub     edi,dir_base
                shl     edi,20
                push    edi
                pop     cx bx
                pop     es
                mov     ss:[esp+4*4],bx
                mov     ss:[esp+4*6],cx
                popad
        no_mapping_needed:
        pop     ds
        iretd

        no_pageable:
        pop     es
        popad
        or      byte ptr ss:[esp+4*3],1
        jmp     no_mapping_needed

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Real Mode Call-Back Services
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
ASSUME  DS:txdata
callback_alloc:
        push    eax ebx ecx edx
        mov     bx,ds
        mov     ds,cs:D16_SEL
        mov     ecx,NUM_RM_CALLBACKS
        xor     eax,eax
        search_a_free_callback:
                cmp     [eax+callback_data.callback_state],FREE_CB
                jz      cb_found
                add     eax,size TCallback
        loop    search_a_free_callback
        ; error: no more free call-back addresses
        pop     edx ecx ebx eax
        or      byte ptr ss:[esp+4*2],1
        iretd

        cb_found:
        mov     [eax+callback_data.callback_state],ALLOCATED_CB    ; now it's allocated
        mov     [eax+callback_data.pm_proc_to_call.loffset],esi
        mov     [eax+callback_data.pm_proc_to_call.selector],bx
        mov     [eax+callback_data.rm_call_struc.loffset],edi
        mov     [eax+callback_data.rm_call_struc.selector],es
        mov     ds,bx   ; segment restore
        pop     edx ecx
        mov     cx,txcode
        ; OFFSET = (EAX / size TCallback) * size CallbackSlot
        mov     ebx,size TCallback
        xor     edx,edx
        div     ebx
        mov     ebx,offset end_of_cb_address-offset callback_address_table
        mul     ebx
        add     eax,offset callback_address_table
        mov     edx,eax         ; CX:DX -> call-back address
        pop     ebx eax
        and     byte ptr ss:[esp+4*2],0FEh
        iretd
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
callback_free:
        push    eax ebx ecx edx
		cmp		cx,txcode
		jnz		cb_not_found
        mov     bx,offset callback_address_table
        xor     eax,eax
        mov     ecx,NUM_RM_CALLBACKS
        search_address:
                cmp     bx,dx
                jz     	address_found
                add     bx,offset end_of_cb_address-offset callback_address_table
                add     eax,size TCallback
        loop    search_address
		cb_not_found:
        pop     edx ecx ebx eax
        or      byte ptr ss:[esp+4*2],1
        iretd

        address_found:
        push    ds
        mov     ds,cs:D16_SEL
        mov     [eax+callback_data.callback_state],FREE_CB ; now it's free
        pop     ds
        pop     edx ecx ebx eax
        and     byte ptr ss:[esp+4*2],0FEh
        iretd

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Real Mode Interrupt Management Functions
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
get_rm_int_vec:
        push    ds ebx
        mov     ds,cs:D32_SEL
        cmp     bl,8
        jb      no_irq_fixup_rm1
        cmp     bl,15
        ja      no_irq_fixup_rm1
                add     bl,NEW_IRQ0_IRQ7_BASE-8        ; irq's redireccionadas
        no_irq_fixup_rm1:
        movzx   ebx,bl
        mov     dx,[ebx*4]
        mov     cx,[ebx*4+2]
        pop     ebx ds
        and     byte ptr ss:[esp+4*2],0FEh
        iretd
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
set_rm_int_vec:
        push    ds
        cmp     bl,8
        jb      no_irq_fixup_rm2
        cmp     bl,15
        ja      no_irq_fixup_rm2
                add     bl,NEW_IRQ0_IRQ7_BASE-8        ; irq's redireccionadas
        no_irq_fixup_rm2:
        movzx   ebx,bl
        mov     ds,cs:D32_SEL
        mov     [ebx*4],dx
        mov     [ebx*4+2],cx
        pop     ds
        and     byte ptr ss:[esp+4*2],0FEh
        iretd
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
lock_unlock_mem:
		and		byte ptr ss:[esp+4*2],0FEh
		iretd

