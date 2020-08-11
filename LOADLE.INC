;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
; file: loadle.inc
;
;  Loads LE executable files. Used by TX32 extender.
;
;       by Xavier Rubio Jansana, a.k.a. Teknik / #Trouble# Team, July 1998
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

DOS_MEMORY          equ     1           ; Memory types
XMS_MEMORY          equ     2

E_FILE_ERROR        equ     0           ; Error codes
E_NOT_LE            equ     1
E_NOT_ENOUGH_MEM    equ     2
E_UNSUPORTED_FEATURE equ    3

;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
; Loads an LE file. First of all it skips the stub, and then loads it.
;
;  parameters:
;    edx - pointer to file name to load
;    ds - selector with zero base
;    es - selector of 16-bit data segment
;
;  returns:
;    cf - set on error
;    eax - offset of entry point
;
;  modifies:
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

LoadLE          proc

        cld

        mov     al, READ_ONLY
        call    openfile                ; Opens the file and seeks LE header
        jc      of_error_loading

        ; ds, es -> 16-bit data segment

        mov     ds, [cs:D16_SEL]
        mov     [handle], eax
        mov     ebx, eax
        mov     ecx, 2
        mov     edx, offset le_hdr
        call    read
        jc      error_loading

        mov     [le_offset], 0          ; Default offset

        cmp     [le_hdr.signature], "ZM"; Let's see if it has an stub
        je      it_has_stub
        cmp     [le_hdr.signature], "MZ"
        jne     check_le

    it_has_stub:
        mov     al, SEEK_SET            ; Get LE's possible offset
        mov     edx, 3ch
        call    seek
        jc      error_loading

        mov     edx, offset le_offset
        mov     ecx, 2
        call    read
        jc      error_loading

        mov     al, SEEK_SET            ; Skip stub
        mov     edx, [le_offset]
        call    seek
        jc      error_loading

        mov     ecx, 2                  ; Get new header's signature
        mov     edx, offset le_hdr
        call    read
        jc      error_loading

    check_le:
        cmp     [le_hdr.signature], "EL"; Check if it's an LE header
        jne     not_LE

        mov     ecx, size le_hdr - 2    ; Ok. It's an LE header
        mov     edx, offset le_hdr + 2
        call    read
        jc      error_loading

    ; Up to here we have LE's header and starting offset. We continue
    ; loading it.

        mov     al, SEEK_SET            ; Load object table
        mov     edx, [le_hdr.off_object_tbl]
        add     edx, [le_offset]
        call    seek
        jc      error_loading

        mov     eax, [le_hdr.obj_tbl_entries]
        imul    eax, size OBJECT
        mov     ecx, eax
        mov     edx, offset object_tbl
        call    read
        jc      error_loading

    ; Get memory for all the objects. This must be done BEFORE the fixup.

        mov     esi, offset object_tbl  ; Start walking object table
        mov     [obj_handles_ptr], 0
        mov     eax, [le_hdr.obj_tbl_entries]
        mov     [obj_count], eax
        mov     [num_obj_handles], 0

        ; ds -> 16-bit data segment
        ; es -> flat data segment (to do mem zeroing)

        mov     es, [cs:DATA_SEL]

    get_memory:

            mov     ebx, [esi.virtual_size]     ; Get memory (first try ext mem)

;  Allocates a block of extended memory.
;
;In:
;  AX     = 0501h
;  BX:CX  = size of block in bytes (must be non-zero)
;
;Out:
;  if successful:
;    carry flag clear
;    BX:CX  = linear address of allocated memory block
;    SI:DI  = memory block handle (used to resize and free block)
;
;  if failed:
;    carry flag set

            push    esi
            mov     cx, bx                      ; bx:cx = size in bytes
            shr     ebx, 16
            mov     ax, 501h
            int     31h
            pop     eax
            jc      not_enough_ext_mem

            mov     edx, [obj_handles_ptr]      ; Save the handle
            shl     esi, 16
            mov     si, di
            mov     [object_nfo.obj_handles + edx * 8 + edx], esi
            mov     [object_nfo.mem_type + edx * 8 + edx], XMS_MEMORY
            inc     [obj_handles_ptr]

            mov     esi, eax                    ; Restore ptr to object table!

            mov     edi, ebx                    ; Get the real address @@
            shl     edi, 16
            mov     di, cx                      ; edi = ptr to memory block
            mov     [object_nfo.obj_offset + edx * 8 + edx], edi

            jmp     we_have_it

        not_enough_ext_mem:

;  Allocates low memory through DOS function 48h and allocates it a descriptor.
;
;In:
;  AX     = 0100h
;  BX     = paragraphs to allocate
;
;Out:
;  if successful:
;    carry flag clear
;    AX     = real mode segment address
;    DX     = protected mode selector for memory block
;
;  if failed:
;    carry flag set
;    AX     = DOS error code
;    BX     = size of largest available block

            push    ebx                         ; bx = # paragraphs
            add     ebx, 15
            shr     ebx, 4
            mov     ax, 100h
            int     31h
            pop     ebx
            jc      not_enough_memory

            mov     ebx, [obj_handles_ptr]      ; Save the "handle"
            mov     word ptr [object_nfo.obj_handles + ebx * 8 + ebx], dx
            mov     [object_nfo.mem_type + ebx * 8 + ebx], DOS_MEMORY
            inc     [obj_handles_ptr]

            movzx   edi, ax                     ; Get the real address @@
            shl     edi, 4                      ; edi = ptr to memory block
            mov     [object_nfo.obj_offset + ebx * 8 + ebx], edi

        we_have_it:

            mov     ecx, [esi.virtual_size]     ; Zero-fill the memory
            push    ecx
            shr     ecx, 2
            xor     eax, eax
            rep     stosd
            pop     ecx
            and     ecx, 3
            jz      end_of_fill
            rep     stosb

        end_of_fill:

            inc     [num_obj_handles]
            add     esi, size OBJECT

        dec     [obj_count]
        jnz     get_memory

    ; We have memory to load the pages. Load them!

        mov     esi, offset object_tbl  ; Start walking object table
        mov     [obj_handles_ptr], 0
        mov     eax, [le_hdr.obj_tbl_entries]
        mov     [obj_count], eax

    walk_object_table:

            ; ds -> 16-bit data segment
            ; es -> who cares?

            mov     eax, [esi.page_map_index]
            dec     eax                         ; It's 1-based
            mov     [curr_page], eax
            mul     [le_hdr.page_size]
            mov     edx, eax
            add     edx, [le_hdr.off_data_pages]; Offset from begin of file
            mov     al, SEEK_SET
            mov     ebx, [handle]
            call    seek
            jc      error_loading

            mov     eax, [obj_handles_ptr]
            mov     edx, [object_nfo.obj_offset + eax * 8 + eax]
            mov     edi, [esi.num_pages]
            inc     [obj_handles_ptr]

            ; ds -> flat data segment
            ; es -> 16-bit data segment

            mov     ds, [cs:DATA_SEL]
            mov     es, [cs:D16_SEL]

            or      edi, edi
            jz      no_pages

        next_page:

                mov     ecx, [es:le_hdr.page_size]
                inc     [es:curr_page]                  ; Increment beforehand!

                mov     eax, [es:curr_page]
                cmp     eax, [es:le_hdr.memory_pages]   ; File's last page?
                jne     last_object_page?

                mov     ecx, [es:le_hdr.bytes_last_page]

                jmp     continue_loading

            last_object_page?:

                cmp     edi, 1                          ; Object's last page?
                jne     continue_loading

                mov     ecx, [es:le_hdr.page_size]
                dec     ecx
                and     ecx, [es:esi.virtual_size]

            continue_loading:

                call    read
                jc      error_loading

                add     edx, [es:le_hdr.page_size]

            dec     edi
            jnz     next_page

        no_pages:

    ; 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
    ; TODO: create descriptors for each object (???)
    ; 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

        ; Now we will fixup each page

            ; ds -> 16-bit data segment   (ds, es inverted!)
            ; es -> flat data segment

            mov     ds, [cs:D16_SEL]
            mov     es, [cs:DATA_SEL]

            mov     edx, [obj_handles_ptr]      ; Get current object ptr
            dec     edx

            mov     eax, [object_nfo.obj_offset + edx * 8 + edx]
            mov     [curr_page_off], eax        ; Offset of current page

            mov     edx, [esi.page_map_index]
            dec     edx                         ; It's 1-based

            shl     edx, 2                      ; File ptr to current fixup
            add     edx, [le_offset]            ;  page table entry
            add     edx, [le_hdr.off_fixup_pag]
            mov     [curr_fixup_pag], edx

            mov     edi, [esi.num_pages]
            mov     ebx, [handle]

            or      edi, edi
            jz      no_pages_to_fixup

        fixup_next_page:

                mov     edx, [curr_fixup_pag]           ; Fixup a page
                mov     al, SEEK_SET
                call    seek
                jc      error_loading

                mov     edx, offset off_1st_fixup_rec
                mov     ecx, 4
                call    read
                jc      error_loading

                mov     edx, offset off_1st_fixup_rec_next_page
                mov     ecx, 4
                call    read
                jc      error_loading

                mov     edx, [off_1st_fixup_rec]
                add     edx, [le_hdr.off_fixup_rec]
                add     edx, [le_offset]
                mov     al, SEEK_SET
                call    seek
                jc      error_loading

            next_fixup_entry:

                    mov     eax, [off_1st_fixup_rec_next_page]
                    cmp     [off_1st_fixup_rec], eax
                    jae     end_fixup

                    mov     edx, offset source_type
                    mov     ecx, 4
                    call    read
                    jc      error_loading

                    add     [off_1st_fixup_rec], 4      ; "Loop counter" :(

                    test    [target_flags], 3           ; Only internal reference
                    jnz     unsuported_feature

                    mov     [object_num], 0
                    mov     ecx, 1
                    test    [target_flags], 40h         ; Object number size
                    jz      load_obj_num
                    inc     ecx

                load_obj_num:
                    mov     edx, offset object_num
                    call    read
                    jc      error_loading

                    add     [off_1st_fixup_rec], eax    ; "Loop counter" :(

                    cmp     [source_type], 7            ; Only 32-bit offset &
                    jne     maybe_16bit_sel             ;  16-bit selectors

                ; 컴컴� 32-bit offset 컴컴�

                    mov     ecx, 2
                    test    [target_flags], 10h         ; Target offset size
                    jz      load_target_off
                    mov     ecx, 4

                load_target_off:
                    mov     edx, offset target_off
                    call    read
                    jc      error_loading

                    add     [off_1st_fixup_rec], eax    ; "Loop counter" :(

                    mov     ecx, [target_off]
                    test    [target_flags], 10h         ; Target offset size
                    jnz     dont_do_zero_extent
                    movzx   ecx, word ptr [target_off]  ; Zero extent, not sign
                                                        ;  extent !!!!
                dont_do_zero_extent:

                    movzx   eax, [object_num]           ; Do the fixup
                    dec     eax                         ; It's 1-based
                    add     ecx, [object_nfo.obj_offset + eax * 8 + eax]
                    movsx   eax, [source_off]
                    add     eax, [curr_page_off]

                    mov     [es:eax], ecx

                    jmp     fixed_up

                ; 컴컴� 16-bit selector 컴컴�

                maybe_16bit_sel:

                    cmp     [source_type], 2            ; 16-bit selector?
                    jne     unsuported_feature

                    movzx   eax, [object_num]           ; Code or data?
                    dec     eax                         ; It's 1-based
                    mov     eax, [object_nfo.object_flags + eax * 8 + eax]
                    test    eax, 4                      ; Executable segment
                    jz      it_is_data
                    mov     cx, [cs:CODE_SEL]           ; a little hack :) @@
                    jmp     we_have_sel_type
                it_is_data:
                    mov     cx, [cs:DATA_SEL]
                we_have_sel_type:

                    movsx   eax, [source_off]
                    add     eax, [curr_page_off]

                    mov     word ptr [es:eax], cx

                fixed_up:

                jmp     next_fixup_entry

            end_fixup:

                mov     eax, [le_hdr.page_size]
                add     [curr_page_off], eax
                add     [curr_fixup_pag], 4

            dec     edi
            jnz     fixup_next_page

        no_pages_to_fixup:

            add     esi, size OBJECT

        dec     [obj_count]             ; Iterate for all entries
        jnz     walk_object_table

        mov     ebx, [handle]           ; Close file
        mov     ah, 3eh
        int     21h

        mov     ebx, [le_hdr.object_cs] ; Calculate entry point
        dec     ebx                     ; It's 1-based!
        mov     eax, [object_nfo.obj_offset + ebx * 8 + ebx]
        add     eax, [le_hdr.offset_eip]

        clc
        ret

    ; Return error codes

    of_error_loading:                   ; Special case: error opening file
        mov     eax, E_FILE_ERROR
        ret

    error_loading:
        mov     eax, E_FILE_ERROR
        jmp     do_cleanup

    not_LE:
        mov     eax, E_NOT_LE
        jmp     do_cleanup

    not_enough_memory:
        mov     eax, E_NOT_ENOUGH_MEM
        jmp     do_cleanup

    unsuported_feature:
        mov     eax, E_UNSUPORTED_FEATURE
        jmp     do_cleanup

    ; Close files & free memory

    do_cleanup:

        push    eax

        mov     ds, [cs:D16_SEL]

        mov     ebx, [handle]           ; Close file
        mov     ah, 3eh
        int     21h

        mov     ecx, [num_obj_handles]
        xor     ebx, ebx

    lle_free_memory:                    ; Free all memory blocs

            or      ecx, ecx
            jz      lle_end_freeing

            cmp     [object_nfo.mem_type + ebx], DOS_MEMORY
            jne     lle_free_xms_mem

            mov     dx, word ptr [object_nfo.obj_handles + ebx]
            mov     ax, 101h
            int     31h

            jmp     lle_continue_freeing

        lle_free_xms_mem:

            mov     esi, [object_nfo.obj_handles + ebx]

            mov     di, si
            shr     esi, 16
            mov     ax, 502h
            int     31h

        lle_continue_freeing:

            add     ebx, size OBJNFO

        dec     ecx
        jmp     lle_free_memory

    lle_end_freeing:

        pop     eax
        stc
        ret

LoadLE          endp

