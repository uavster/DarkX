.386p
.model flat
.stack 100h
.code
start:
        mov     ax,200h
        mov     bl,16h
        int     31h
        mov     ax,201h
        mov     bl,80h
        int     31h

        xor     ah,ah
        int     80h

        mov     ah,4Ch
        int     21h
end start
