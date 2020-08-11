ASSUME  DS:txcode32
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
; Shows the state of heap.
;  esi -> heap info     
;  modifies:
;    everything but si, di :)
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

show_heap       proc

        mov     ebx, [esi.start]       ; start walking trought heap

        mov     edx, offset m_heap      ; print size of heap
        mov     eax,txcode32
        shl     eax,4
        add     edx,eax
        mov     ah, 9
        int     21h
        mov     eax, [esi.hlen]
        push    ebx
        call    dword2hex
        pop     ebx
        mov     edx, offset m_CRLF
        mov     eax,txcode32
        shl     eax,4
        add     edx,eax
        mov     ah, 9
        int     21h

sh_lp:  mov     edx, offset m_off       ; print offset
        mov     eax,txcode32
        shl     eax,4
        add     edx,eax
        mov     ah, 9
        int     21h
        mov     eax, ebx
        push    ebx
        call    dword2hex
        pop     ebx

        mov     edx, offset m_len       ; print length
        mov     eax,txcode32
        shl     eax,4
        add     edx,eax
        mov     ah, 9
        int     21h
        mov     eax, [ebx.blen]
        push    ebx
        call    dword2hex
        pop     ebx

        mov     eax, dword ptr [ebx.status]     ; print status, more & signature
        mov     edx,txcode32
        shl     edx,4
        mov     [edx+m_sms_st], al
        mov     [edx+m_sms_m], ah
        shr     eax, 16
        mov     word ptr [edx+m_sms_sg], ax
        mov     edx, offset m_sms
        mov     eax,txcode32
        shl     eax,4
        add     edx,eax
        mov     ah, 9
        int     21h

        mov     eax, [ebx.prev]         ; print previous
        push    ebx
        call    dword2hex
        pop     ebx

        mov     edx, offset m_nxt       ; print next
        mov     eax,txcode32
        shl     eax,4
        add     edx,eax
        mov     ah, 9
        int     21h
        mov     eax, [ebx.next]
        push    ebx
        call    dword2hex
        pop     ebx

        mov     edx, offset m_CRLF      ; print CRLF
        mov     eax,txcode32
        shl     eax,4
        add     edx,eax
        mov     ah, 9
        int     21h

        cmp     [ebx.more], 'E'         ; more blocks?
        je      sh_end
        mov     ebx, [ebx.next]
        jmp     sh_lp

sh_end: xor     ah, ah                  ; wait a moment... :)
        int     16h
        ret

show_heap       endp

;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
; Prints a dword.
;
;  parameters:
;    eax - dword to print
;
;  modifies:
;    ebx, cl, dl
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

dword2hex       proc

        mov     cl, 8                   ; eight nibbles...
dh_lp:  rol     eax, 4
        mov     ebx, eax
        and     ebx, 0fh
        push    eax
        mov     eax,txcode32
        shl     eax,4
        add     ebx,eax
        mov     ah, 2
        mov     dl, [ebx + too_lazy]
        int     21h
        pop     eax
        dec     cl
        jnz     dh_lp
        ret

dword2hex       endp

;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

init    db      "Initializing memory manager...", 13, 10, "$"

m_tmr_m db      13, 10, "malloc test: $"
m_tmr_f db      13, 10, "free test: $"
m_tmr2  db      " ticks.", 13, 10, "$"

m_fmax  db      13, 10, "Maximum free block size: $"
m_ftotal db     " bytes.", 13, 10, "Total available memory:  $"
m_mend  db      " bytes.", 13, 10, 13, 10, "$"

m_memok db      "Memory check is successful!", 13, 10, "$"
m_memerr db     "Memory check is unsuccessful!", 13, 10, "$"

m_heap  db      13, 10, "Heap size: $"

m_off   db      13, 10, "Starting offset of block: $"
m_len   db      13, 10, "Length of block: $"
m_sms   db      13, 10, "Status (allocated/free): "
m_sms_st db     0, " More (more/end): "
m_sms_m db      0, " Signature: "
m_sms_sg db     0, 0, 13, 10, "Previous block: $"
m_nxt   db      13, 10, "Next block: $"
m_CRLF  db      13, 10, "$"

too_lazy db     "0123456789abcdef"

;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
