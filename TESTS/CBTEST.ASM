.386p
.model flat
.stack 400h
.code
Trm_reg         STRUC
        rm_edi          dd ?
        rm_esi          dd ?
        rm_ebp          dd ?
                        dd ?
        rm_ebx          dd ?
        rm_edx          dd ?
        rm_ecx          dd ?
        rm_eax          dd ?
        rm_flags        dw ?
        rm_es           dw ?
        rm_ds           dw ?
        rm_fs           dw ?
        rm_gs           dw ?
        rm_ip           dw ?
        rm_cs           dw ?
        rm_sp           dw ?
        rm_ss           dw ?
ENDS
;        INCLUDE utils.inc
ErrorExit       MACRO error_msg
LOCAL   skip_msg,msg
        jnc     skip_msg
        push    ss
        pop     ds
        mov     ah,9
        mov     edx,offset msg
        int     21h
        jmp     exit_from_here
msg     db error_msg,"$"
skip_msg:
ENDM

start:  jmp short main
        db "WATCOM"
main:
;        mov     ax,100h
;        mov     bx,100
;        int     31h
;        ErrorExit "Arg! no puedo con la convencionaaaaaal... No pueeeedo!"
;        mov     es,dx
;        xor     edi,edi
        mov     segmento,ds
        push    ds
        pop     es
        mov     edi,offset rmcallstruc
        push    cs
        pop     ds
        mov     ax,303h
        mov     esi,offset proctocall
        int     31h
        ErrorExit "No me guttta tu call-back!"
        mov     ax,201h
        mov     bl,80h
        int     31h
        ErrorExit "Interruciong? y ezo que eh???"

        bucle:
                int     80h
        mov     ah,1
        int     16h
        jz      bucle
        xor     ah,ah
        int     16h

;        Exit
        exit_from_here:
        mov     ah,4Ch
        int     21h

proctocall:
        mov     ds,cs:segmento
        mov     al,hihi
        mov     ah,1eh
        mov     ds:[0b8000h+4*160],ax
        inc     al
        cmp     al,3Ah
        jnz     nada
                mov     al,30h
        nada:
        mov     hihi,al

        cld
        lodsw
        mov     es:[edi.rm_ip],ax
        lodsw
        mov     es:[edi.rm_cs],ax
        lodsw
        mov     es:[edi.rm_flags],ax
        add     es:[edi.rm_sp],2*3
        iretd
segmento        dw ?

.data
hihi            db 30h
rmcallstruc     dd 100 dup(?)
end start
