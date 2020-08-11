;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
init_dos_heap   proc
; DS = D32_SEL
        mov     eax,txcode32
        shl     eax,4
        mov     esi,offset dos_pointers
        add     esi,eax
        cmp     byte ptr [esi],0
        jz      no_more_heaps   ; con 0 bloques, salimos
        add     eax,offset dos_heap_info
        mov     ebx,[esi+1]
        mov     ecx,[esi+5]
        call    initheap
        xor     ecx,ecx
        mov     cl,[esi]
        dec     ecx
        jz      no_more_heaps
        add     esi,9
        add_heaps:
                push    ecx
                mov     eax,txcode32
                shl     eax,4
                add     eax,offset dos_heap_info
                mov     ebx,[esi]
                mov     ecx,[esi+4]
                call    addheap
                pop     ecx
                add     esi,8
        loop    add_heaps
        no_more_heaps:
        ret
init_dos_heap   endp
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
init_extended_heap        proc
; DS = D32_SEL
        ASSUME  DS:txcode32
        mov     esi,txcode32
        shl     esi,4
        movzx   ecx,byte ptr [esi+extended_pointers]
        or      ecx,ecx
        jz      extended_init_done      ; si hay 0 bloques, salimos
        push    ecx
        mov     eax,offset extended_heap_info
        add     eax,esi
        mov     ebx,dword ptr [esi+extended_pointers+1]
        mov     ecx,dword ptr [esi+extended_pointers+5]
        call    initheap
        pop     ecx
        dec     ecx
        jz      extended_init_done
                make_xms_heap:
                        push    ecx
                        mov     eax,txcode32
                        shl     eax,4
                        add     eax,offset extended_heap_info
                        mov     ebx,dword ptr [esi+9+extended_pointers]
                        mov     ecx,dword ptr [esi+13+extended_pointers]
                        call    addheap
                        add     esi,8
                        pop     ecx
                loop    make_xms_heap
                ASSUME  DS:txdata
        extended_init_done:
        ret
init_extended_heap        endp
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
