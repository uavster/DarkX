.386p
.model flat
.stack 100h
.code
start:
        mov     bx,cs
        mov     ax,6
        int     31h
        jc      error
        shl     ecx,16
        mov     cx,dx
        mov     ebx,0b8000h
        sub     ebx,ecx
        mov     dword ptr [ebx+4*160],'juju'

        fin:
        mov ah,4Ch
        int 21h

        error:
        mov     ah,9
        mov     edx,offset error_msg
        int     21h
        jmp     fin

error_msg       db "Error getting segment base$"
end start
