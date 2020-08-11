ASSUME  DS:txdata
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
ret_real_raw:
; INPUT -> DS = D16_SEL
        cli

        mov     ax,ds
        mov     es,ax
	mov     fs,ax
	mov     gs,ax
	mov     ss,ax

	lidt    fword ptr idt_real

	mov     eax,cr0
	and     al,0FEh
	mov     cr0,eax

saltito db 0eah
	dw ?
	dw ?

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

set_pm_raw:
; INPUT -> DS = D16_SEL
        ; ponemos la GDT y la IDT
        db      66h
        lgdt    fword ptr gdt_reg
        lidt    fword ptr idt_reg
        ; vamos a P.MODE
        smsw    ax
        or      ax,1
        lmsw    ax
        jmp     next_label      ; carga la prefetch queue
        next_label:
        jmp     fword ptr prot_code_address
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
alloc_xms_mem MACRO
        mov     ax,txcode32
        mov     es,ax
        ASSUME  ES:txcode32
        mov     byte ptr es:extended_pointers,0
        xor     di,di
        cont_adding_xms:
        mov     ah,8
        xor     bl,bl
        call    dword ptr xms_entry_point
        or      bl,bl
        jnz     xms_alloc_done
        push    ax
        mov     dx,ax
        mov     ah,9
        call    dword ptr xms_entry_point
        pop     cx
        or      ax,ax
        jz      xms_alloc_done
        mov     word ptr es:[di+xms_handles],dx
        inc     byte ptr es:extended_pointers
        mov     ah,0Ch
        call    dword ptr xms_entry_point
        or      ax, ax
        jnz     page_is_locked  ; si no la bloquea, no la a¤adimos a la heap
                dec     byte ptr es:extended_pointers
                jmp     cont_adding
        page_is_locked:
        shl     edx,16
        mov     dx,bx
        movzx   ecx,cx
        shl     ecx,10
        mov     dword ptr es:[di+extended_pointers+1],edx
        mov     dword ptr es:[di+extended_pointers+1+4],ecx
        add     di,8
        cmp     di,8*100
        je      xms_alloc_done
        jmp     cont_adding_xms
        xms_alloc_done:
ENDM
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
free_xms_mem    MACRO
        ASSUME  DS:txcode32,ES:txdata
        mov     ax,txcode32
        mov     ds,ax
        mov     ax,txdata
        mov     es,ax
        mov     si,offset xms_handles
        movzx   cx,byte ptr extended_pointers
        or      cx,cx
        jz      xms_mem_freed
        free_xms_blocks:
                mov     dx,[si]
                mov     ah,0Dh          ; UNLOCK block
                call    dword ptr es:xms_entry_point
                mov     ah,0Ah          ; FREE block
                call    dword ptr es:xms_entry_point
                add     si,8
        loop    free_xms_blocks
        xms_mem_freed:
        ASSUME  DS:txdata
ENDM
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
look_for_extended_beginning     MACRO
; buscamos el principio de la extendida libre con el m‚todo VDISK
; OUTPUT -> EAX = extended memory beginning
        xor     ax,ax
        mov     fs,ax
        dec     ax
        mov     gs,ax
        mov     fs,fs:[19h*4+2]
        mov     eax,100000h
        cmp     dword ptr fs:[12h],'SIDV'
        jnz     method_2
        mov     edx,fs:[12h+4]
        shl     edx,8
        shr     edx,8
        cmp     edx,'V K'
        jnz     method_2
        mov     eax,fs:[2Ch]
        shl     eax,8
        shr     eax,8
        method_2:
        cmp     dword ptr gs:[16+3],'SIDV'
        jnz     extended_init_found
        cmp     byte ptr gs:[16+3+4],'K'
        jnz     extended_init_found
        movzx   edx,word ptr gs:[16+1Eh]     ; +16 para el siguiente segmento
        shl     edx,10
        add     edx,100000h
        cmp     edx,eax
        jbe     extended_init_found
        xchg    edx,eax
        extended_init_found:

ENDM
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
