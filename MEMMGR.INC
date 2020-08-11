; NOTE: All procedures are called with DS = 4Gb data segment

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; file: memmgr.asm
;
;  Memory manager for Trouble's DOS extender.
;
;       by Teknik/Trouble, November of 1997
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

;  All block lengths are a multiple of 4.

THEAP   struc

start   dd      ?               ; ptr to first block of heap
hlen    dd      ?               ; length of heap

        ends

TBLOCK  struc

blen    dd      ?               ; block length, without this header
status  db      ?               ; 'A' allocated, 'F' free
more    db      ?               ; 'M' more blocks, 'E' end of heap
sign    dw      ?               ; block signature "TX", not reversed as TASM does
prev    dd      ?               ; offset of previous block header, origin DS
next    dd      ?               ; offset of next block header, origin DS

        ends

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Initializes heap. All other procedures assume that this is the first proc
; called, just before doing something.
;
;  parameters:
;    eax - ptr to heap info (8 bytes required)  
;    ebx - address of memory (first block)
;    ecx - length of heap (including headers) (=length of first block)
;
;  modifies:
;    ecx, eax, ebx, edx
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

initheap        proc

        mov     edx, ebx                ; align heap start...
        add     ebx, 3
        and     bl, 0fch
        sub     edx, ebx                ; ...and adjust the length too
        sub     ecx, edx
        add     ecx, 3
        and     cl, 0fch

        mov     [eax.hlen], ecx
        sub     ecx, 16
        mov     [ebx.blen], ecx
        mov     dword ptr [ebx.status], 'XTEF'  ; free, end, signature...
        xor     ecx, ecx                ; ... (TASM reverses this strings :( )
        mov     [ebx.prev], ecx
        mov     [ebx.next], ecx

        mov     [eax.start], ebx        ; saves ptr to the first block

        ret

initheap        endp

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Adds another block of heap, as heap may not be continuous. This can be
; done at any time.
;
;  parameters:
;    eax - ptr to heap info
;    ebx - address of the new block
;    ecx - length of new block (including headers)
;
;  modifies:
;    eax, ebx, ecx, edx
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

addheap proc

        mov     edx, ebx                ; align new block start...
        add     ebx, 3
        and     bl, 0fch
        sub     edx, ebx                ; ...and adjust the length too
        sub     ecx, edx
        add     ecx, 3
        and     cl, 0fch

        mov     edx, [eax.start]
        add     [eax.hlen], ecx         ; calc new heap length
ah_lp:  cmp     [edx.more], 'E'         ; searchs last block
        je      ah_add
        mov     edx, [edx.next]
        jmp     short ah_lp

ah_add: mov     [edx.more], 'M'         ; add the new free block
        mov     [edx.next], ebx
        sub     ecx, 16
        mov     [ebx.blen], ecx
        mov     dword ptr [ebx.status], 'XTEF'
        mov     [ebx.prev], edx
        mov     [ebx.next], 0

        add     ebx, 16                 ; ebx -> ptr to *data area*
        call    free                    ; quick way to possibly collide blocks

        ret

addheap endp

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Allocates a memory block.
;
;  parameters:
;    eax - ptr to heap info
;    ecx - length of block to allocate in bytes
;
;  returns:
;    ebx - ptr to memory or NULL if not enought memory
;    ecx - real length of block (rounded up to a four multiple)
;
;  modifies:
;    eax
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

malloc  proc

        mov     ebx, [eax.start]        ; search a free block
        add     ecx, 3                  ; round up to multiple of 4
        and     cl, 0fch
m_lp:   cmp     byte ptr [ebx.status], 'F'
        jne     m_next
        cmp     dword ptr [ebx.blen], ecx
        jb      m_next

        add     ecx, 16                 ; allocates block
        cmp     dword ptr [ebx.blen], ecx    ; if the remainder is less than
        jb      dont_split              ; ... 16 don't split the block

        mov     eax, [ebx.blen]         ; ebx + ecx -> ptr to new block
        sub     eax, ecx                ; ecx = len of allocated block + 16
        mov     [ebx + ecx.blen], eax
        mov     eax, dword ptr [ebx.status]     ; move status, more and sign
        mov     dword ptr [ebx + ecx.status], eax
        mov     [ebx + ecx.prev], ebx
        mov     eax, [ebx.next]
        mov     [ebx + ecx.next], eax

        mov     [ebx.more], 'M'         ; more blocks
        mov     [ebx.next], ebx         ; adjust ptr to next block
        add     [ebx.next], ecx

        sub     ecx, 16
        mov     [ebx.blen], ecx

dont_split:

        mov     [ebx.status], 'A'       ; allocated block

        mov     ecx, [ebx.blen]         ; ecx -> length of block
        add     ebx, 16                 ; ebx -> data area of block

        ret

m_next: cmp     byte ptr [ebx.more], 'M'
        jne     m_end
        mov     ebx, [ebx.next]
        jmp     short m_lp

m_end:  xor     ebx, ebx                ; not enought memory

        ret

malloc  endp

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Coalesces two memory blocks.
;
;  parameters:
;    eax - ptr to lower block
;    ebx - ptr to upper block
;
;  modifies:
;    ecx
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

coalesce macro label

        mov     ecx, eax                ; are two blocks adjacent?
        add     ecx, [eax.blen]
        add     ecx, 16
        cmp     ecx, ebx
        jne     label

        mov     cl, [ebx.more]          ; copy more flag
        mov     [eax.more], cl
        mov     ecx, [ebx.next]         ; relink blocks
        mov     [eax.next], ecx
        mov     [ecx.prev], eax
        mov     ecx, [ebx.blen]         ; readjust length
        add     ecx, 16
        add     [eax.blen], ecx

        endm

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Frees a memory block.
;
;  parameters:
;    ebx - ptr to data area of block
;
;  modifies:
;    eax, ecx, ebx
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

free    proc

        sub     ebx, 16                 ; ebx -> ptr to block header

        mov     [ebx.status], 'F'       ; free it

        mov     eax, [ebx.prev]         ; coalesce with previous block
        or      eax, eax
        jz      f_next
        cmp     [eax.status], 'F'       ; is free? ...
        jne     f_next
        coalesce f_next                 ; ... yes, coalesce it
        mov     ebx, eax                ; now we have only first free block

f_next: mov     eax, [ebx.next]         ; coalesce with next block
        or      eax, eax
        jz      f_end
        cmp     [eax.status], 'F'       ; is free? ...
        jne     f_end
        xchg    eax, ebx
        coalesce f_end                  ; ... yes, coalesce it

f_end:
        ret

free    endp

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Resizes a memory block.
;
;  parameters:
;    eax - ptr to heap info (needen for possible mallocs)
;    ebx - ptr to data area of block
;    ecx - new block size
;
;  returns:
;    ebx - new block ptr, zero if not enought memory
;    ecx - new block size, rounded up to a multiple of four
;
;  modifies:
;    eax, edx, esi, edi
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

realloc proc

        sub     ebx, 16                 ; ebx -> ptr to block header

        add     ecx, 3                  ; round size up
        and     cl, 0fch
        cmp     ecx, [ebx.blen]         ; is new block bigger or smaller?
        ja      r_big

        ; ÍÍÍÍÍ new block size <= old block size ÍÍÍÍÍ

        push    ebx                     ; block won't change, so save ebx

        mov     eax, [ebx.next]         ; have we a free block after?
        or      eax, eax
        jz      r_newf
        cmp     [eax.status], 'F'
        jne     r_newf                  ; no, make a new free block (if we can)

        ; we have a free block after the resized block, so we move it

        mov     edx, ebx                ; move start of free block
        add     edx, ecx
        add     edx, 16
        mov     [ebx.next], edx
        push    [ebx.blen]
        mov     [ebx.blen], ecx
        mov     ebx, edx                ; ebx -> ptr to free block
        pop     edx
        add     edx, [eax.blen]
        sub     edx, ecx                ; edx = new length of block
        mov     [ebx.blen], edx
        mov     edx, dword ptr [eax.status]
        mov     dword ptr [ebx.status], edx
        mov     edx, [eax.prev]
        mov     [ebx.prev], edx
        mov     edx, [eax.next]
        mov     [ebx.next], edx
        or      edx, edx
        jz      r_s_end
        mov     [edx.prev], ebx
        jmp     short r_s_end

        ; we create a new free block if we can (i.e. free space >= 16)

r_newf: mov     edx, [ebx.blen]         ; calc the difference
        sub     edx, ecx
        sub     edx, 16                 ; we can't resize the block 'cos free
        jc      r_s_end                 ;  space is less than 16 bytes...
        mov     [16 + ebx + ecx.blen], edx      ; make a new block
        mov     eax, dword ptr [ebx.status]
        mov     dword ptr [16 + ebx + ecx.status], eax
        mov     [16 + ebx + ecx.status], 'F'

        mov     [ebx.more], 'M'         ; update the resized block
        mov     [ebx.blen], ecx

        mov     eax, [ebx.next]         ; relink the heap
        mov     [16 + ebx + ecx.next], eax
        mov     [ebx.next], ebx
        add     [ebx.next], ecx
        add     [ebx.next], 16
        mov     [16 + ebx + ecx.prev], ebx
        or      eax, eax
        jz      r_s_end
        mov     [eax.prev], ebx
        add     [eax.prev], ecx
        add     [eax.prev], 16

r_s_end:
        pop     ebx                     ; ebx -> ptr to data area of resized
        add     ebx, 16                 ;  block
        jmp     r_end

        ; ÍÍÍÍÍ new block size > old block size ÍÍÍÍÍ

r_big:  push    eax                     ; save ptr to heap info
        mov     eax, [ebx.next]         ; check next block to see if is a free
        cmp     [eax.status], 'F'       ;  one and if it's big enought
        jne     r_malloc

        mov     edx, ecx
        sub     edx, [ebx.blen]
        sub     edx, 16                 ; we can use the block header too
        cmp     [eax.blen], edx
        jb      r_malloc

        add     esp, 4                  ; discard ptr to heap info

        ; we have a free block just after the resized block and it's big
        ;  enought, so we use it

        add     edx, 16                 ; use all the block or only a little
        cmp     [eax.blen], edx
        jbe     r_kill

        mov     [ebx.blen], ecx          ; update lengths
        sub     edx, [eax.blen]
        neg     edx                     ; eax -> ptr old, ebx -> ptr reloc.
        add     ecx, ebx                ; ecx -> ptr to new block location
        add     ecx, 16
        mov     [ecx.blen], edx

        mov     edx, dword ptr [eax.status]     ; copy old block to new block
        mov     dword ptr [ecx.status], edx

        mov     [ebx.next], ecx         ; relink lists
        mov     edx, [eax.prev]
        mov     [ecx.prev], edx
        mov     edx, [eax.next]
        mov     [ecx.next], edx
        mov     ecx, [ebx.blen]         ; restore size
        or      edx, edx
        jz      r_end
        mov     [edx.prev], eax
        jmp     short r_end

r_kill: mov     edx, [eax.blen]         ; use all the block
        add     edx, 16
        add     [ebx.blen], edx
        mov     edx, [eax.next]         ; relink blocks
        mov     [ebx.next], edx
        or      edx, edx
        jz      r_nonext
        mov     [edx.prev], ebx
r_nonext:
        mov     dl, [eax.more]
        mov     [ebx.more], dl

        jmp     short r_end

        ; we haven't a free block after the resized block, or it's not big
        ;  enought, so we call malloc to get memory if we can, copy old
        ;  block to new block and free old block

r_malloc:
        pop     eax                     ; restore ptr to heap info
        push    ebx                     ; we have to allocate another block
        call    malloc                  ;  and copy data to it
        or      ebx, ebx                ; not enought memory, return zero ptr
        jz      r_notenought
        mov     edi, ebx                ; copy block
        pop     edx                     ; don't destroy ebx -> ptr to new block
        add     edx, 16                 ; edx must point to data area
        mov     esi, edx
        cld
        push    ecx
        shr     ecx, 2                  ; ecx is allways a multiple of four
        rep     movsd
        xchg    edx, ebx                ; free old block
        call    free
        mov     ebx, edx
        pop     ecx                     ; end with ebx -> ptr, ecx -> length
        jmp     short r_end

r_notenought:
        add     esp, 4                  ; just exit with ebx = 0

r_end:  
        ret

realloc endp

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Gets memory info.
;
;  parameters:
;    eax - ptr to heap info
;
;  returns:
;    eax - maximum available block
;    ecx - total available memory
;
;  modifies:
;    ebx
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

getmeminfo      proc

        mov     ebx, [eax.start]        ; walk thought the heap
        xor     eax, eax
        xor     ecx, ecx
gmi_lp: cmp     [ebx.status], 'F'       ; is free?
        jne     gmi_nxt
        cmp     [ebx.blen], eax         ; get maximum available block
        jb      gmi_c
        mov     eax, [ebx.blen]
gmi_c:  add     ecx, [ebx.blen]
gmi_nxt:
        cmp     [ebx.more], 'E'
        je      gmi_end
        mov     ebx, [ebx.next]
        jmp     short gmi_lp

gmi_end:

        ret

getmeminfo      endp
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
