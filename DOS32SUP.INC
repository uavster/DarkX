;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
; DOS32 basic support routines
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
ASSUME  DS:txdata
dos32_sys_info:
        push    ds
        mov     ds,cs:D16_SEL
        mov     ebx,program_offset
        mov     edx,exe_size
        mov     esi,psp_offset
        mov     edi,env_offset
        mov     ecx,program_name_ptr
        mov     ax,disk
        pop     ds
        iretd
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
dos32_info:
        call    determine_sys   ; DL = system type
        xor     eax,eax         ; DOS32 version = 0.0 cause nothing is implemented
        mov     bx,cs:D32_SEL   ; BX = selector 32 bits base 0 l죑. 4Gb
        iretd
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴�
