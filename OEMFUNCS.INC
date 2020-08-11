TX32_VERSION    EQU 0098

ASSUME  DS:txdata
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; TX32 OEM functions
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
get_sys_info:
; OUTPUT -> AX = cpu info
;               AH = cpu family ; AL = b7..4: model
;                                      b3..0: stepping
;           ( CPUID return code where available and 0300h or 0400h for 386
;            and 486 respectively )
;           BL = FPU type
;                ( 00h = not present ; 02h = 287 ; 03h = 387 ; 04h = 487 )
;           CL = system type
;                ( 01h = RAW ; 02h = XMS ; 03h = VCPI ; 04h = DPMI )
        push    ds
        call    determine_sys
        mov     cl,dl
        mov     ds,cs:D16_SEL
        mov     ax,cpu_info
        mov     bl,fpu_info
        pop     ds
        iret

determine_sys    proc
; OUTPUT -> DL = system type with output format
;           ( 01 = RAW , 02 = XMS , 04 = VCPI , 08 = DPMI )
        push    ds
        mov     ds,cs:D16_SEL
        xor     eax,eax
        mov     al,system_type
        mov     dl,cs:[eax+sys_type_vals]
        cmp     dl,1
        jnz     sys_determined
                cmp     xms_entry_point,0
                jz      sys_determined
                inc     dl
        sys_determined:
        pop     ds
        ret
sys_type_vals   db 1,4,8
determine_sys   endp
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
get_program_info:
; INPUT  -> DS:ESI -> buffer for TX32 information
;           ECX = size of buffer (for not overflowing with later versions)
;
; OUTPUT -> AX = TX32 version in binary (v0.98 -> AX=0062h, for example)
;                       AH = major version
;                       AL = minor version
;           DS:ESI -> info buffer filled with a maximum of ECX bytes
;
;               Format of info buffer for TX32 v0.98:
;
;               Offset  Size    Description
;               00h     DWORD   Program base (zero with LE's)
;               04h     DWORD   PSP offset relative to program base
;               08h     DWORD   Environment offset rel. to prog. base
;               0Ch     DWORD   ASCIIZ exe name offset rel. to prog. base
;               10h     WORD    Zero based 4Gb data selector
;               12h     WORD    Program based 4Gb data selector
;               14h     WORD    PSP based 100h bytes data selector
;               16h     WORD    Real mode disk buffer segment
;                               (used for disk data interchange between real
;                                mode and protected mode)

        push    ds es esi edi ecx
        or      ecx,ecx
        jz      program_info_done
        cmp     ecx,24
        jbe     length_ok
                mov     ecx,24
        length_ok:
        push    ds esi
        pop     edi es
        mov     ds,cs:D16_SEL
        mov     esi,offset exe_info_struc
        cld
        rep     movsb
        program_info_done:
        mov     ax,TX32_VERSION         ; version number
        pop     ecx edi esi es ds
        iretd
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
