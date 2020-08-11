ASSUME  DS:txdata
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
; DLINK Executable Loader
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
ASSUME  ES:txdata
        mov     es,cs:D16_SEL
        mov     edx,es:program_name_ptr
        mov     ds,cs:D32_SEL
        mov     ax,3D00h        ; open for reading
        int     21h
        jc      load_error

        mov     ebx,eax
        mov     ecx,txcode32
        sub     ecx,txdata
        shl     ecx,4
        add     ecx,offset code_end+512  ; ECX = stub size (+512 for EXE header)
        mov     edx,ecx
        shr     ecx,16
        mov     ah,42h          ; like this it's smaller than AX,4200h ;)
        xor     al,al           ; <---
        int     21h
        jc      load_error

        mov     ah,3Fh
        mov     edx,txcode32
        shl     edx,4
        add     edx,offset print_buffer
        mov     ecx,28h         ; DLINK exe header size
        int     21h
        jc      load_error

        cmp     dword ptr cs:print_buffer,'madA'
        mov     edx,offset bad_exec_error_msg
        jnz     pm_error
        test    byte ptr cs:print_buffer+24h,1
        mov     edx,offset comp_exe_error_msg
        jnz     pm_error
        push    ebx
        mov     ebx,dword ptr cs:print_buffer+14h
        mov     eax,dword ptr cs:print_buffer+20h
        shl     eax,2
        add     ebx,eax         ; size of mem. needed + size of reloc. table
        mov     cx,bx
        shr     ebx,16

        ; alojamos memoria para el programa
        mov     ax,501h
        int     31h
        mov     edx,offset exe_size_error_msg
        jnc     have_mem_for_exe
        ; si no hay extendida, usamos memoria DOS
        mov     ax,100h
        shl     ebx,16
        mov     bx,cx
        cmp     ebx,655360      ; 640 Kb
        jae     exe_size_error
        shr     ebx,4
        int     31h
        jc      exe_size_error
        movzx   eax,ax
        shl     eax,4
        mov     cx,ax
        shr     eax,16
        mov     bx,ax
        have_mem_for_exe:
        shl     ebx,16
        mov     bx,cx
        mov     edx,ebx         ; EDX = linear address of buffer

        pop     ebx
        push    edx
        mov     ax,4201h
        mov     ecx,dword ptr cs:print_buffer+0Ch
        sub     ecx,28h
        mov     dx,cx
        shr     ecx,16
        int     21h
        jc      load_error

        pop     edx
        mov     ecx,dword ptr cs:print_buffer+10h
        mov     eax,dword ptr cs:print_buffer+20h
        shl     eax,2
        add     ecx,eax         ; exec. size + relocation table
        mov     ah,3Fh
        int     21h
        jc      load_error

        ; closing archive...
        mov     ah,3Eh
        xor     al,al
        int     21h

        ; arreglamos selectores de c줰igo y datos del programa
        mov     dword ptr es:program_offset,edx
        mov     word ptr es:code_desc+2,dx
        mov     word ptr es:data_desc+2,dx
        shr     edx,16
        mov     byte ptr es:code_desc+4,dl
        mov     byte ptr es:code_desc+7,dh
        mov     byte ptr es:data_desc+4,dl
        mov     byte ptr es:data_desc+7,dh

        test    es:system_type,2
        jz      no_dpmi_sel_fixup
                mov     ax,0Ch
                mov     bx,cs:CODE_SEL
                mov     edi,offset code_desc
                int     31h
                jc      desc_alloc_error
                mov     bx,cs:DATA_SEL
                mov     edi,offset data_desc
                int     31h
                jc      desc_alloc_error

        no_dpmi_sel_fixup:
        ; rellenamos info sobre el EXE
        ; tama쨚
        mov     eax,txcode32
        sub     eax,txdata
        shl     eax,4
        add     eax,offset code_end+512  ; EAX = stub size
        add     eax,dword ptr cs:print_buffer+8
        mov     es:exe_size,eax
        ; offset PSP
        mov     ebx,es:program_offset
        sub     es:psp_offset,ebx
        ; offset ENV
        sub     es:env_offset,ebx
        ; offset nombre
        sub     es:program_name_ptr,ebx

        mov     ax,cs:DATA_SEL
        mov     ds,ax
        mov     es,ax
        mov     fs,ax
        mov     gs,ax
        cli
        mov     ss,ax
        mov     esp,dword ptr cs:print_buffer+1Ch       ; INITIAL ESP
        sti

        mov     ecx,dword ptr cs:print_buffer+20h
        or      ecx,ecx
        jz      no_fixup_needed
        mov     esi,dword ptr cs:print_buffer+10h
        mov     eax,dword ptr cs:DATA_SEL
        selector_fixup:
                mov     edi,[esi]
                mov     [edi],ax
                add     esi,4
        loop    selector_fixup

        no_fixup_needed:        
        push    dword ptr cs:CODE_SEL
        push    dword ptr cs:print_buffer+18h           ; INITIAL EIP
        retf

exe_size_error:
        mov     edx,offset exe_size_error_msg
        jmp     pm_error
desc_alloc_error:
        mov     edx,offset desc_alloc_error_msg
        jmp     pm_error
load_error:
        mov     edx,offset load_error_msg
        jmp     pm_error



